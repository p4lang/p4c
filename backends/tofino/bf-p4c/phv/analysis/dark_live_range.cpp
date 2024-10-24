/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/phv/analysis/dark_live_range.h"

#include <sys/types.h>

#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/table_layout.h"
#include "bf-p4c/phv/analysis/live_range_shrinking.h"
#include "bf-p4c/phv/utils/liverange_opti_utils.h"

bool DarkLiveRange::overlaps(const int num_max_min_stages, const DarkLiveRangeEntry &range1,
                             const DarkLiveRangeEntry &range2) {
    const int DEPARSER = num_max_min_stages + 1;
    bitvec f1Uses;
    bitvec f2Uses;
    unsigned index = 0;
    for (int i = 0; i <= DEPARSER; i++) {
        for (unsigned j : {READ, WRITE}) {
            StageAndAccess stageAndAccess = std::make_pair(i, PHV::FieldUse(j));
            if (range1.count(stageAndAccess)) {
                if (!range1.at(stageAndAccess).second) {
                    // Ignore dark uses for the purposes of deciding overlay
                    f1Uses.setbit(index);
                    LOG_DEBUG5(TAB2 "Setting f1 bit " << index << ", stage " << i << ": "
                                                      << PHV::FieldUse(j));
                } else {
                    LOG_DEBUG5(TAB3 "Not setting f1 bit " << index << ", stage " << i << ": "
                                                          << "DARK " << PHV::FieldUse(j));
                }
            }
            if (range2.count(stageAndAccess)) {
                if (!range2.at(stageAndAccess).second) {
                    f2Uses.setbit(index);
                    LOG_DEBUG5(TAB2 "Setting f2 bit " << index << ", stage " << i << ": "
                                                      << PHV::FieldUse(j));
                } else {
                    LOG_DEBUG5(TAB3 "Not setting f2 bit " << index << ", stage " << i << ": "
                                                          << "DARK " << PHV::FieldUse(j));
                }
            }
            ++index;
        }
    }
    LOG_DEBUG4(TAB2 "Field 1 use: " << f1Uses << ", Field 2 use: " << f2Uses);
    bitvec combo = f1Uses & f2Uses;
    LOG_DEBUG4(TAB2 "combo: " << combo << ", overlaps? " << (combo.popcount() != 0));
    return (combo.popcount() != 0);
}

bool DarkLiveRange::increasesDependenceCriticalPath(const IR::MAU::Table *use,
                                                    const IR::MAU::Table *init) const {
    const int maxStages = dg.max_min_stage_per_gress[use->gress];
    int use_stage = dg.min_stage(use);
    int init_stage = dg.min_stage(init);
    int init_dep_tail = dg.dependence_tail_size_control_anti(init);
    LOG_DEBUG5(TAB2 "use table: " << use->name << ", init table: " << init->name);
    LOG_DEBUG5(TAB2 "use_stage: " << use_stage << ", init_stage: " << init_stage
                                  << ", init_dep_tail: " << init_dep_tail);
    if (use_stage < init_stage) return false;
    int new_init_stage = use_stage + 1;
    LOG_DEBUG5(TAB2 "new_init_stage: " << new_init_stage << ", maxStages: " << maxStages);
    LOG_DEBUG5(TAB3 "MinStages: useTable(" << *(PhvInfo::minStages(use).begin()) << ")  initTable("
                                           << *(PhvInfo::minStages(init).begin()) << ")");

    if (new_init_stage + init_dep_tail > maxStages) return true;
    return false;
}

Visitor::profile_t DarkLiveRange::init_apply(const IR::Node *root) {
    livemap.clear();
    overlay.clear();
    doNotInitActions.clear();
    doNotInitToDark.clear();
    doNotInitTables.clear();
    BUG_CHECK(dg.finalized, "Dependence graph is not populated.");
    // For each use of the field, parser implies stage `dg.max_min_stage + 2`, deparser implies
    // stage `dg.max_min_stage + 1` (12 for Tofino), and a table implies the corresponding
    // dg.min_stage.
    DEPARSER = dg.max_min_stage + 1;
    livemap.setDeparserStageValue(DEPARSER);
    LOG_DEBUG1("Deparser is at " << DEPARSER << ", max stage: " << dg.max_min_stage);
    return Inspector::init_apply(root);
}

bool DarkLiveRange::preorder(const IR::MAU::Table *tbl) {
    uint64_t totalKeySize = 0;
    for (const auto *key : tbl->match_key) {
        le_bitrange bits;
        const auto *field = phv.field(key->expr, &bits);
        if (!field) continue;
        totalKeySize += bits.size();
    }
    if (!tbl->match_table) return true;
    const auto *sz = tbl->match_table->getSizeProperty();
    if (!sz) return true;
    if (!sz->fitsUint64()) return true;
    uint64_t tableSize = sz->asUint64();
    uint64_t totalBits = totalKeySize * tableSize;
    if (totalBits > Memories::SRAM_ROWS * Memories::SRAM_COLUMNS * Memories::SRAM_DEPTH * 128) {
        doNotInitTables.insert(tbl);
        LOG_DEBUG4(TAB1 "Do not insert initialization in table: " << tbl->name);
    }
    return true;
}

bool DarkLiveRange::preorder(const IR::MAU::Action *act) {
    GetActionRequirements ghdr;
    act->apply(ghdr);
    if (ghdr.is_hash_dist_needed()) {
        LOG_DEBUG2(TAB1 "Cannot initialize at action " << act->name
                                                       << " because it requires "
                                                          "the hash distribution unit.");
        doNotInitActions.insert(act);
    }

    auto *tbl = findContext<IR::MAU::Table>();
    if (tbl && tbl->getAnnotation("no_field_initialization"_cs)) {
        doNotInitActions.insert(act);
        LOG_DEBUG3("Pragma @no_field_initialization found for action: "
                   << act->externalName() << " in table " << tbl->externalName());
    }

    return true;
}

void DarkLiveRange::setFieldLiveMap(const PHV::Field *f) {
    LOG_DEBUG4(TAB1 "Setting live range for field " << f);
    // minUse = earliest stage for uses of the field.
    // maxUse = latest stage for uses of the field.
    // minDef = earliest stage for defs of the field.
    // maxDef = latest stage for defs of the field.
    // Set the min values initially to the deparser, and the max values to the parser initially.
    for (const FieldDefUse::locpair &use : defuse.getAllUses(f->id)) {
        const IR::BFN::Unit *use_unit = use.first;
        if (use_unit->is<IR::BFN::ParserState>() || use_unit->is<IR::BFN::Parser>() ||
            use_unit->to<IR::BFN::GhostParser>()) {
            // Ignore parser use if field is marked as not parsed.
            if (notParsedFields.count(f)) {
                LOG_DEBUG4(TAB1 "  Ignoring field " << f << " use in parser");
                continue;
            }
            LOG_DEBUG4(TAB1 "  Used in parser.");
            livemap.addAccess(f, PARSER, READ, use_unit, false);
        } else if (use_unit->is<IR::BFN::Deparser>()) {
            // Ignore deparser use if field is marked as not deparsed.
            if (notDeparsedFields.count(f)) continue;
            LOG_DEBUG4(TAB1 "  Used in deparser.");
            livemap.addAccess(f, DEPARSER, READ, use_unit, false);
        } else if (use_unit->is<IR::MAU::Table>()) {
            const auto *t = use_unit->to<IR::MAU::Table>();
            int use_stage = dg.min_stage(t);
            LOG_DEBUG4(TAB1 "  Used in stage " << use_stage << " in table " << t->name);
            livemap.addAccess(f, use_stage, READ, use_unit, !defuse.hasNonDarkContext(use));
            if (!defuse.hasNonDarkContext(use)) LOG_DEBUG4("\t\tCan use in a dark container.");
        } else {
            BUG("Unknown unit encountered %1%", use_unit->toString());
        }
    }

    // Set live range for every def of the field.
    for (const FieldDefUse::locpair &def : defuse.getAllDefs(f->id)) {
        const IR::BFN::Unit *def_unit = def.first;
        // If the field is specified as pa_no_init, and it has an uninitialized read, we ignore the
        // compiler-inserted parser initialization.
        if (noInitFields.count(f)) {
            if (def.second->is<ImplicitParserInit>()) {
                LOG_DEBUG4("  Ignoring def of field " << f
                                                      << " with uninitialized read and def in "
                                                         "parser state "
                                                      << DBPrint::Brief << def_unit);
                continue;
            }
        }
        // If the field is not specified as pa_no_init and has a def in the parser, check if the def
        // is of type ImplicitParserInit, and if it is, we can safely ignore this def.
        if (def_unit->is<IR::BFN::ParserState>() || def_unit->is<IR::BFN::Parser>() ||
            def_unit->to<IR::BFN::GhostParser>()) {
            if (def.second->is<ImplicitParserInit>()) {
                LOG_DEBUG4(TAB2 "  Ignoring implicit parser init.");
                continue;
            }
            if (notParsedFields.count(f)) {
                LOG_DEBUG4(TAB2 "  Ignoring because field set to not parsed");
            } else {
                LOG_DEBUG4(TAB1 "  Field defined in parser.");
                livemap.addAccess(f, PARSER, WRITE, def_unit, false);
                continue;
            }
        } else if (def_unit->is<IR::BFN::Deparser>()) {
            if (notDeparsedFields.count(f)) continue;
            LOG_DEBUG4(TAB1 "  Defined in deparser.");
            livemap.addAccess(f, DEPARSER, WRITE, def_unit, false);
        } else if (def_unit->is<IR::MAU::Table>()) {
            const auto *t = def_unit->to<IR::MAU::Table>();
            int def_stage = dg.min_stage(t);
            LOG_DEBUG4(TAB1 "  Defined in stage " << def_stage << " in table " << t->name);
            livemap.addAccess(f, def_stage, WRITE, def_unit,
                              !(nonMochaDark.isNotDark(f, t).isWrite()));
            if (!(nonMochaDark.isNotDark(f, t).isWrite()))
                LOG_DEBUG4(TAB2 "Can use in a dark container");
        } else {
            BUG("Unknown unit encountered %1%", def_unit->toString());
        }
    }
}

void DarkLiveRange::end_apply() {
    // If there are no stages required, do not run this pass.
    if (dg.max_min_stage < 0) return;
    // Set of fields whose live ranges must be calculated.
    ordered_set<const PHV::Field *> fieldsConsidered;
    for (const PHV::Field &f : phv) {
        if (clot.fully_allocated(&f)) continue;
        if (f.is_deparser_zero_candidate()) continue;
        // Ignore unreferenced fields because they are not allocated anyway.
        if (!uses.is_referenced(&f)) continue;
        // Ignore POV fields.
        if (f.pov) continue;
        // If a field is marked by pa_no_overlay, do not consider it for dark overlay.
        if (noOverlay.get_no_overlay_fields().count(&f)) continue;
        fieldsConsidered.insert(&f);
    }
    for (const auto *f : fieldsConsidered) setFieldLiveMap(f);
    if (LOGGING(1)) LOG_DEBUG1(livemap.printDarkLiveRanges());
    for (const auto *f1 : fieldsConsidered) {
        // Do not dark overlay ghost fields.
        if (f1->isGhostField()) continue;
        for (const auto *f2 : fieldsConsidered) {
            if (f1 == f2) continue;
            if (!noOverlay.can_overlay(f1, f2)) continue;
            if (f2->isGhostField()) continue;
            // No overlay possible if fields are of different gresses.
            if (f1->gress != f2->gress) {
                overlay(f1->id, f2->id) = false;
                continue;
            }
            if (!livemap.count(f1) || !livemap.count(f2)) {
                // Overlay possible because one of these fields is not live at all.
                overlay(f1->id, f2->id) = true;
                continue;
            }
            auto &access1 = livemap.at(f1);
            auto &access2 = livemap.at(f2);
            LOG_DEBUG3(TAB1 "(" << f1->name << ", " << f2->name << ")");
            if (!overlaps(dg.max_min_stage, access1, access2)) {
                LOG_DEBUG4(TAB1 "Overlay possible between " << f1 << " and " << f2);
                overlay(f1->id, f2->id) = true;
            }
        }
    }
}

// Returns pair of {READ, WRITE} accesses per stage for the slices in @fields
// If more than one slice have the same access in the same stage, return
// std::nullopt
// ---
std::optional<DarkLiveRange::ReadWritePair> DarkLiveRange::getFieldsLiveAtStage(
    const ordered_set<PHV::AllocSlice> &fields, const int stage) const {
    const PHV::AllocSlice *readField = nullptr;
    const PHV::AllocSlice *writtenField = nullptr;
    bool found_dark_slice = false;
    for (auto &sl : fields) {
        //  Add hack to prevent dark overlaid fields being
        //        considered twice. This may need to be updated once
        //        we introduce spills from dark to other containers
        if (sl.container().is(PHV::Kind::dark)) {
            if (found_dark_slice) {
                LOG_DEBUG4("Warning: Found more than one dark slices");
                return std::nullopt;
            }

            found_dark_slice = true;
            continue;
        }

        if (livemap.hasAccess(sl.field(), stage, READ)) {
            if (readField != nullptr) {
                //  This will reject overlays with packed fields that have
                // READs in the same stage. We should add the capability
                // to overlay with packed fields that have smae-stage READs
                LOG_DEBUG4("Slices " << readField << " and " << sl << " both read in stage "
                                     << ((stage == DEPARSER) ? "deparser" : std::to_string(stage)));
                return std::nullopt;
            }
            readField = &sl;
        }
        if (livemap.hasAccess(sl.field(), stage, WRITE)) {
            if (writtenField != nullptr) {
                LOG_DEBUG4("Slices " << writtenField << " and " << sl << " both written in stage "
                                     << stage);
                return std::nullopt;
            }
            writtenField = &sl;
        }
    }
    if (readField) LOG_DEBUG5("Adding slice " << *readField << " READ in stage " << stage);
    if (writtenField) LOG_DEBUG5("Adding slice " << *writtenField << " WRITE in stage " << stage);
    return std::make_pair(readField, writtenField);
}

bool DarkLiveRange::validateLiveness(const OrderedFieldSummary &rv) const {
    const OrderedFieldInfo *lastSlice = nullptr;
    static PHV::FieldUse read(READ);
    static PHV::FieldUse write(WRITE);
    for (auto &info : rv) {
        if (lastSlice == nullptr) {
            lastSlice = &info;
            continue;
        }
        if (info.minStage.second == read) {
            // If the min stage is a read, then the initialization must happen in the previous
            // stage. Therefore, we need to calculate a real min stage that is 1 less than the min
            // stage calculated earlier.
            int realMinStage = info.minStage.first - 1;
            if (lastSlice->maxStage.second == write && lastSlice->maxStage.first >= realMinStage) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Found overlapping slices in terms of live range: ");
                LOG_FEATURE("alloc_progress", 5,
                            TAB3 << lastSlice->field.field()->name << " ["
                                 << lastSlice->minStage.first << lastSlice->minStage.second << ", "
                                 << lastSlice->maxStage.first << lastSlice->maxStage.second << "]");
                LOG_FEATURE("alloc_progress", 5,
                            TAB3 << info.field.field()->name << " [" << realMinStage << "W, "
                                 << info.maxStage.first << info.maxStage.second << "] (original: ["
                                 << info.minStage.first << info.minStage.second << ", "
                                 << info.maxStage.first << info.maxStage.second << "])");
                return false;
            }
        }
        lastSlice = &info;
    }
    return true;
}

std::optional<DarkLiveRange::OrderedFieldSummary> DarkLiveRange::produceFieldsInOrder(
    const ordered_set<PHV::AllocSlice> &slices, bool &onlyReadRefs) const {
    LOG_DEBUG5("Producing slices in order : " << slices);
    OrderedFieldSummary rv;

    ordered_set<PHV::AllocSlice> ParsedOnlySlices(slices);
    const PHV::AllocSlice *lastField = nullptr;
    for (int i = -1; i <= DEPARSER; i++) {
        auto fieldsLiveAtStage = getFieldsLiveAtStage(slices, i);
        if (!fieldsLiveAtStage) return std::nullopt;
        if (fieldsLiveAtStage->first != nullptr) {
            // Update ParsedOnlySlices
            for (auto itr = ParsedOnlySlices.begin(); itr != ParsedOnlySlices.end();) {
                if (itr->field()->id == fieldsLiveAtStage->first->field()->id)
                    itr = ParsedOnlySlices.erase(itr);
                else
                    ++itr;
            }

            auto &readAccess = livemap.at(fieldsLiveAtStage->first->field(), i, READ);
            if (lastField != fieldsLiveAtStage->first) {
                lastField = fieldsLiveAtStage->first;
                OrderedFieldInfo info(*lastField, std::make_pair(i, PHV::FieldUse(READ)),
                                      readAccess);
                LOG_DEBUG5(TAB1 "Read Info: " << info);
                rv.push_back(info);
            } else {
                rv[rv.size() - 1].addAccess(std::make_pair(i, PHV::FieldUse(READ)), readAccess);
                LOG_DEBUG5(TAB1 "Adding Read Access: " << DBPrint::Brief << readAccess.first);
            }
        }
        if (fieldsLiveAtStage->second != nullptr) {
            // Update ParsedOnlySlices
            for (auto itr = ParsedOnlySlices.begin(); itr != ParsedOnlySlices.end();) {
                if (itr->field()->id == fieldsLiveAtStage->second->field()->id)
                    itr = ParsedOnlySlices.erase(itr);
                else
                    ++itr;
            }

            // Slices have also write references
            onlyReadRefs = false;
            auto &writeAccess = livemap.at(fieldsLiveAtStage->second->field(), i, WRITE);
            if (lastField != fieldsLiveAtStage->second) {
                lastField = fieldsLiveAtStage->second;
                OrderedFieldInfo info(*lastField, std::make_pair(i, PHV::FieldUse(WRITE)),
                                      writeAccess);
                LOG_DEBUG5(TAB1 "Write Info: " << info);
                rv.push_back(info);
            } else {
                rv[rv.size() - 1].addAccess(std::make_pair(i, PHV::FieldUse(WRITE)), writeAccess);
                LOG_DEBUG5(TAB1 "Adding Write Access: " << DBPrint::Brief << writeAccess.first);
            }
        }
    }

    LOG_DEBUG5(TAB1 "Number of alloc slices sorted by liveness: " << rv.size());
    for (auto &info : rv) {
        std::stringstream ss;
        ss << TAB2 << info.field << " : [" << info.minStage.first << info.minStage.second << ", "
           << info.maxStage.first << info.maxStage.second << "].  Units: ";
        for (const auto *u : info.units)
            ss << DBPrint::Brief << "(" << u->thread() << ")" << u << " ";
        LOG_DEBUG5(ss.str());
    }

    if (!validateLiveness(rv)) return std::nullopt;
    if (ParsedOnlySlices.size() == 0) {
        return rv;
    }

    // Handle case of fields that have only Parser references (local parser fields)
    lastField = nullptr;
    auto sItr = slices.end();
    OrderedFieldSummary rv2;

    auto fieldsLiveAtStage = getFieldsLiveAtStage(ParsedOnlySlices, -1);
    if (!fieldsLiveAtStage) return std::nullopt;

    if (fieldsLiveAtStage) {
        if (fieldsLiveAtStage->first != nullptr) {
            lastField = fieldsLiveAtStage->first;
            sItr = slices.find(*(fieldsLiveAtStage->first));
            BUG_CHECK(sItr != slices.end(), "Cannot find parsed-only slice %1%", *sItr);
            auto &readAccess = livemap.at(sItr->field(), -1, READ);
            OrderedFieldInfo info(*sItr, std::make_pair(-1, PHV::FieldUse(READ)), readAccess);
            LOG_DEBUG5(TAB3 "Parser Read Info: " << info);
            rv2.push_back(info);
        }

        if (fieldsLiveAtStage->second != nullptr) {
            sItr = slices.find(*(fieldsLiveAtStage->second));
            BUG_CHECK(sItr != slices.end(), "Cannot find parsed-only slice %1%", *sItr);
            auto &writeAccess = livemap.at(sItr->field(), -1, WRITE);
            if (lastField != fieldsLiveAtStage->second) {
                lastField = fieldsLiveAtStage->second;
                OrderedFieldInfo info(*sItr, std::make_pair(-1, PHV::FieldUse(WRITE)), writeAccess);
                LOG_DEBUG5(TAB3 "Parse Write Info: " << info);
                rv2.push_back(info);
            } else {
                rv2[rv2.size() - 1].addAccess(std::make_pair(-1, PHV::FieldUse(WRITE)),
                                              writeAccess);
                LOG_DEBUG5(TAB3 "Adding Parser Write Access: " << DBPrint::Brief
                                                               << writeAccess.first);
            }
        }
    }

    for (auto info : rv) {
        rv2.push_back(info);
    }
    LOG_DEBUG5(
        TAB1 "Number of updated with parser-only alloc slices sorted by liveness: " << rv2.size());
    return rv2;
}

// Ignore reach conflicts if they are due to PARSER/DEPARSER units or if
// they are on the same table with lastField doing a READ and
// currentField doing a WRITE (Each stage has a READ->WRITE ordering)
// ---
bool DarkLiveRange::ignoreReachCondition(
    const OrderedFieldInfo &currentField, const OrderedFieldInfo &lastField,
    const ordered_set<std::pair<const IR::BFN::Unit *, const IR::BFN::Unit *>> &conflicts) const {
    bool rv = true;
    for (auto &conflict : conflicts) {
        const auto *t1 = conflict.first->to<IR::MAU::Table>();
        const auto *t2 = conflict.second->to<IR::MAU::Table>();
        if (!t1 || !t2 || t1 != t2) return false;
        // Here both the conflicting units are the same.
        LOG_DEBUG6(TAB4 "t1: " << t1->name << ",  t2:" << t2->name);
        bool currentAccessAtTable =
            livemap.hasAccess(currentField.field.field(), dg.min_stage(t1), WRITE);
        bool lastAccessAtTable = livemap.hasAccess(lastField.field.field(), dg.min_stage(t1), READ);
        LOG_DEBUG6(TAB4 "current: " << currentAccessAtTable
                                    << ", lastAccessAtTable: " << lastAccessAtTable);
        // FIXME: Determine right condition
        if (!currentAccessAtTable || !lastAccessAtTable) {
            rv = false;
            break;
        }
    }
    return rv;
}

bool DarkLiveRange::isGroupDominatorEarlierThanFirstUseOfCurrentField(
    const OrderedFieldInfo &currentField, const PHV::UnitSet &doms,
    const IR::MAU::Table *groupDominator) const {
    bool singleDom = (doms.size() == 1);
    if (currentField.minStage.first < dg.min_stage(groupDominator)) return false;
    for (const auto *u : doms) {
        // If there is only one dominator in the list of trimmed dominators, check that the
        // only strict dominator in there is the same as the group dominator. If it is, then it is
        // fine for the initialization point to be at the group dominator.
        const auto *t = u->to<IR::MAU::Table>();
        if (!t) return false;
        LOG_DEBUG5(TAB4 "Group dominator: " << groupDominator->name << ", dom: " << t->name);
        if (singleDom && t == groupDominator) {
            LOG_DEBUG5(TAB4 "Only strict dominator same as group dominator. Initialization ok.");
            return true;
        }
        if (dg.min_stage(groupDominator) > dg.min_stage(t)) {
            LOG_DEBUG5(TAB4 "Group dominator happens in same logical stage or later (stage "
                       << dg.min_stage(groupDominator) << ") than use table " << t->name << " ("
                       << dg.min_stage(t) << ")");
            return false;
        }
    }
    // If group dominator stage is the same as the trimmed dominator, then we need to make sure
    // that the trimmed dominator is a read (and not a write) because we would be inserting an
    // initialization at the group dominator table. The check here makes sure that the group
    // dominator will occur earlier than the min stage for the current field, which effectively
    // implements the invariant above.
    // TODO: This is too conservative. This check is necessary only if we actually need to
    // initialize the field.
    if (currentField.minStage.first == dg.min_stage(groupDominator) &&
        currentField.minStage.second == PHV::FieldUse(READ)) {
        LOG_DEBUG5(TAB4
                   "Initialization at group dominator will happen later than the first "
                   "use of currentField "
                   << currentField.field);
        return false;
    }

    // All the strict dominators are in later stages compared to the group dominator.
    return true;
}

// @fields contains the candidate slice as well as the already
//         allocated slices that will be overlaid with the candidate slice
// @onlyReadCandidates is a flag that is set when the AllocSlices in @fields
//         have only read references
// @return is set to true if:
//         there are write references for AllocSlices in @fields or
//         in slices allocated to bits which will not be involved in the overlay
bool DarkLiveRange::nonOverlaidWrites(const ordered_set<PHV::AllocSlice> &fields,
                                      const PHV::Transaction &alloc, const PHV::Container c,
                                      bool onlyReadCandidates) const {
    bool rv = false;
    std::optional<le_bitrange> overlay_range = std::nullopt;

    // if the AllocSlices in @fields are taking the entire container then
    // there are no non-overlaid writes, i.e. all slices should be
    // contained in @fields
    for (auto f_slc : fields) {
        if (!overlay_range) {
            overlay_range = f_slc.container_slice();
        } else {
            *overlay_range |= f_slc.container_slice();
        }
    }

    LOG_DEBUG5(TAB1 "Overlaid slices range: " << *overlay_range);

    if (overlay_range->size() == static_cast<ssize_t>(c.size())) return rv;

    // Check if there are more allocated slices than included in @fields
    auto alloc_slices = alloc.slices(c);

    // Examine allocated slices not included in @fields
    bool onlyReadAlloc = true;
    ordered_set<PHV::AllocSlice> non_overlap_slices;
    for (auto alloc_slc : alloc_slices) {
        // Only consider allocated slices in container bitrange not
        // entailed in overlay_range
        if (overlay_range->overlaps(alloc_slc.container_slice())) continue;
        non_overlap_slices.insert(alloc_slc);
    }
    auto allocInOrder = produceFieldsInOrder(non_overlap_slices, onlyReadAlloc);

    if (!(onlyReadCandidates && onlyReadAlloc)) rv = true;

    return rv;
}

std::optional<PHV::DarkInitMap> DarkLiveRange::findInitializationNodes(
    const PHV::ContainerGroup &group, const PHV::Container &c,
    const ordered_set<PHV::AllocSlice> &fields, const PHV::Transaction &alloc, bool canUseARA,
    bool prioritizeARAinits) const {
    // If one of the fields has been marked as "do not init to dark" (primarily because we did not
    // find dominator nodes for use nodes of those fields), then return false early.
    for (const auto &sl : fields)
        if (doNotInitToDark.count(sl.field())) return std::nullopt;
    // iterate over stages. gather the stages where each of the slices are alive
    // for initialization, we move pairwise between fields in the container (in increasing order of
    // liveness, where increasing means use in larger stage numbers). find the initialization point
    // for the live-later field and then move to the next pair. A field may appear multiple times
    // for initialization in this scheme (unlike metadata initialization).
    // question: what if multiple slices are alive at the same stage? it can happen when dark
    // overlay mixes with parser overlay and metadata overlay due to live range shrinking.
    bool onlyReadCandidates = true;
    auto fieldsInOrder = produceFieldsInOrder(fields, onlyReadCandidates);
    if (!fieldsInOrder) return std::nullopt;

    // For mocha containers check whether there are writes in
    // non-overlaid container slices; We should not allow overlays then
    if (c.is(PHV::Kind::mocha) && nonOverlaidWrites(fields, alloc, c, onlyReadCandidates)) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB2
                    "Cannot overlay because container is mocha "
                    " and there are writes in non-overlaid container slices");
        return std::nullopt;
    }

    const OrderedFieldInfo *lastField = nullptr;
    unsigned idx = 0;
    PHV::DarkInitMap rv;
    PHV::DarkInitEntry *firstDarkInitEntry = nullptr;
    for (const auto &info : *fieldsInOrder) {
        if (LOGGING(2)) {
            LOG_FEATURE("alloc_progress", 6,
                        TAB1 "Trying to allocate field " << idx << ": " << info.field
                                                         << " in container " << c);
            LOG_DEBUG6(TAB2 "Relevant units:");
            for (const auto *u : info.units) {
                std::stringstream ss;
                ss << TAB2 << DBPrint::Brief << u;
                if (const auto *t = u->to<IR::MAU::Table>())
                    ss << " (Stage " << dg.min_stage(t) << ")";
                LOG_DEBUG2(ss.str());
            }
        }
        if (idx == 0) {
            LOG_DEBUG2(TAB1 "Field with earliest live range: " << info.field
                                                               << " already "
                                                                  "allocated to "
                                                               << c);
            lastField = &info;
            PHV::AllocSlice dest(info.field);
            dest.setLiveness(info.minStage, info.maxStage);
            if (LOGGING(5)) {
                LOG_DEBUG5("\t  A. Adding units to slice " << dest);
                for (auto entry : info.field.getRefs())
                    LOG_DEBUG5("\t\t " << entry.first << " (" << entry.second << ")");
            }

            dest.addRefs(info.field.getRefs());
            firstDarkInitEntry = new PHV::DarkInitEntry(dest);
            firstDarkInitEntry->setNop();
            LOG_DEBUG3(TAB3 "Creating dark init primitive (not pushed): " << *firstDarkInitEntry);
            ++idx;
            continue;
        }
        BUG_CHECK(lastField && lastField->field != info.field,
                  "DarkLiveRange should never see the same field "
                  "consecutively.");
        LOG_DEBUG2(TAB1 "Need to move field: " << info.field << " into container " << c
                                               << ", and move last field " << lastField->field
                                               << " into a dark container");
        LOG_DEBUG2(TAB2 "Live range: (" << info.minStage.first << info.minStage.second << ", "
                                        << info.maxStage.first << info.maxStage.second << ")");
        // Check the uses of fields initialized so far in this container with the uses relevant to
        // this particular field.
        unsigned idx_g = 0;
        for (const auto &g_info : *fieldsInOrder) {
            if (idx_g++ == idx) break;
            if (phv.isFieldMutex(info.field.field(), g_info.field.field())) {
                LOG_DEBUG2(TAB2 "Exclusive with field " << (idx_g - 1) << ": " << g_info.field);
                // FIXME: How does this effect the return vector.
                continue;
            }
            LOG_DEBUG2(TAB2 "Non-exclusive with field " << g_info.field);
            LOG_DEBUG2(TAB2 "Can all defuses of " << info.field << " reach the defuses of "
                                                  << g_info.field << "?");
            auto updatedFlowgraph = update_flowgraph(g_info.units, info.units,
                                                     domTree.getFlowGraph(), alloc, canUseARA);
            auto reach_condition = canFUnitsReachGUnits(info.units, g_info.units, updatedFlowgraph);
            if (reach_condition.size() > 0) {
                bool ignoreReach = ignoreReachCondition(info, g_info, reach_condition);
                if (!ignoreReach) {
                    LOG_FEATURE("alloc_progress", 6,
                                TAB3 "Yes. Therefore, move to dark not possible.");
                    return std::nullopt;
                }
            }
            LOG_DEBUG2(TAB3 "No. Trying to find an initialization node.");
        }

        // Populate the list of dominators for the current live range slice of
        // the field.
        PHV::UnitSet f_nodes;
        for (auto iunit : info.units) {
            // Since dominator analysis is gress specific we ideally should not
            // be seeing fields from different gresses (e.g. ingress and ghost)
            // being considered here. FieldDefUse & CreateLocalThreadInstances
            // should identify and mark the threads accordingly.
            BUG_CHECK(iunit->thread() == info.field.field()->gress,
                      "All units for dominator analysis do not belong to same gress");
            f_nodes.insert(iunit);
        }
        unsigned idx_2 = 0;
        for (const auto &info_2 : *fieldsInOrder) {
            if (idx_2++ <= idx) continue;
            LOG_DEBUG5(TAB3 "Now adding " << info_2.units.size() << " units for " << info_2.field
                                          << " : (" << info_2.minStage.first
                                          << info_2.minStage.second << ", " << info_2.maxStage.first
                                          << info_2.maxStage.second << ")");
            for (auto *u : info_2.units) LOG_DEBUG5(TAB3 << DBPrint::Brief << u);
            for (auto iunit2 : info_2.units) {
                // Skip units which are in a different gress
                if (info.field.field()->gress != iunit2->thread()) continue;
                f_nodes.insert(iunit2);
            }
        }

        if (LOGGING(4)) {
            LOG_DEBUG4(TAB2 << "f_nodes:");

            for (const auto *u : f_nodes) {
                if (u->is<IR::BFN::Parser>() || u->is<IR::BFN::ParserState>() ||
                    u->is<IR::BFN::Deparser>()) {
                    LOG_DEBUG5(TAB3 << DBPrint::Brief << u);
                } else {
                    LOG_DEBUG4(TAB3 << DBPrint::Brief << u << " (stage "
                                    << dg.min_stage(u->to<IR::MAU::Table>()) << ")");
                }
            }
        }

        bool onlyDeparserUse = false;
        bool infoNodesInFnodes = false;
        if (f_nodes.size() == 1) {
            const auto *u = *(f_nodes.begin());
            if (u->is<IR::BFN::Deparser>()) {
                onlyDeparserUse = true;
                LOG_DEBUG2(TAB2 "Initialize in last stage always_init VLIW block.");
            }
        }
        if (!onlyDeparserUse) {
            LOG_DEBUG2(TAB2 "Trimming the list of use nodes in the set of defuses.");
            getTrimmedDominators(f_nodes, domTree);
            if (hasParserUse(f_nodes)) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Defuse units of field "
                                << info.field
                                << " includes the parser. Cannot find initialization point.");
                return std::nullopt;
            }
            // Check if any of the units referencing the current
            // candidate field are part of the trimmed dominator nodes
            for (auto iunit : info.units) {
                if (f_nodes.count(iunit)) {
                    infoNodesInFnodes = true;
                    break;
                }
            }

            if (!infoNodesInFnodes) {
                LOG_DEBUG2(TAB2 " Defuse unit of field "
                           << info.field
                           << " dominated by later"
                              " units. Initializing may impact table placement.");
            }

            if (LOGGING(2)) {
                for (const auto *u : f_nodes) {
                    if (u->is<IR::BFN::Deparser>()) {
                        LOG_DEBUG2(TAB3 << DBPrint::Brief << u);
                    } else {
                        LOG_DEBUG2(TAB3 << DBPrint::Brief << u << " (stage "
                                        << dg.min_stage(u->to<IR::MAU::Table>()) << ")");
                    }
                }
            }
        }
        bool movePreviousToDark = mustMoveToDark(*lastField, *fieldsInOrder);
        if (movePreviousToDark) {
            LOG_DEBUG2(TAB2 "Move the previous field " << lastField->field
                                                       << " into a dark "
                                                          "container after stage "
                                                       << lastField->maxStage.first
                                                       << lastField->maxStage.second);
        } else {
            LOG_DEBUG2(TAB2 "No need to move field " << lastField->field
                                                     << " into a dark container");
        }
        bool initializeCurrentField = onlyDeparserUse;
        if (!onlyDeparserUse) initializeCurrentField = mustInitializeCurrentField(info, f_nodes);
        if (initializeCurrentField) {
            LOG_DEBUG2(TAB2 "Must initialize container "
                       << c << " for current field " << info.field << " after stage "
                       << lastField->maxStage.first << lastField->maxStage.second);
        } else {
            LOG_DEBUG2(TAB2 "No need to initialize container " << c << " for current field "
                                                               << info.field);
        }

        bool initializeFromDark = mustInitializeFromDarkContainer(info, *fieldsInOrder);
        if (initializeFromDark)
            LOG_DEBUG2(TAB2 "Must initialize container " << c << " for current field " << info.field
                                                         << " from dark container after stage "
                                                         << lastField->maxStage.first
                                                         << lastField->maxStage.second);
        if (initializeCurrentField && !initializeFromDark)
            LOG_DEBUG2(TAB2 "Must initialize container "
                       << c << " for current field " << info.field << " using 0 after stage "
                       << lastField->maxStage.first << lastField->maxStage.second);

        if (initializeCurrentField) {
            if (onlyDeparserUse && initializeFromDark) {
                LOG_DEBUG4(TAB2 "   Trying to initialize from dark for deparser-only use");
                auto init = generateInitForLastStageAlwaysInit(info, lastField, rv);
                if (!init) return std::nullopt;
                rv.push_back(*init);
                continue;
            } else if ((fieldsInOrder->size() == 2) && (idx == 1) && canUseARA) {
                // Use of ARA for zero-init of later slice in metadata overlay of 2 slices
                // This is enabled with pragma @pa_prioritize_ara_inits
                LOG_DEBUG4(
                    TAB2 "   Trying to zero initialize:   onlyDeparserUse:" << onlyDeparserUse);
                auto init = generateARAzeroInit(info, lastField, alloc, onlyDeparserUse,
                                                onlyReadCandidates);
                if (init && prioritizeARAinits) {
                    if (firstDarkInitEntry != nullptr) {
                        LOG_DEBUG3(TAB2
                                   "Need to push the first dark init primitive "
                                   "corresponding to "
                                   << *firstDarkInitEntry);
                        rv.push_back(*firstDarkInitEntry);
                    }

                    rv.push_back(*init);
                    continue;
                }
                if (onlyDeparserUse) {
                    LOG_FEATURE("alloc_progress", 5,
                                TAB2
                                "Currently doesn't support"
                                " initialization from 0 in always init block.");
                    return std::nullopt;
                }
                LOG_DEBUG4(TAB3
                           "Did not generate ARA for zero Init, will try using dominator"
                           " table ...");
            }
        }

        bool ARAspill =
            PhvInfo::darkSpillARA && movePreviousToDark && !initializeFromDark && canUseARA;
        const IR::MAU::Table *groupDominator =
            getGroupDominator(info.field.field(), f_nodes, info.field.field()->gress);
        int firstDarkInitMaxStage = 0;

        if (!ARAspill && groupDominator == nullptr) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB2 "Cannot find group dominator to write " << lastField->field
                                                                     << " into a dark container.");
            return std::nullopt;
        }

        while (ARAspill || groupDominator != nullptr) {
            // Check that the group dominator can be used to add the move instruction without
            // lengthening the dependence chain.

            bool goToNextDominator = false;
            bool groupDominatorAfterLastUsePrevField =
                ARAspill ? true
                         : (dg.min_stage(groupDominator) > lastField->maxStage.first) ||
                               ((dg.min_stage(groupDominator) == lastField->maxStage.first) &&
                                lastField->maxStage.second == PHV::FieldUse(READ));
            bool groupDominatorBeforeFirstUseCurrentField =
                ARAspill ? true
                         : isGroupDominatorEarlierThanFirstUseOfCurrentField(info, f_nodes,
                                                                             groupDominator);

            if (!groupDominatorAfterLastUsePrevField) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Cannot find initialization point for "
                                 "previous field "
                                << lastField->field
                                << " because group dominator's"
                                   " stage ("
                                << dg.min_stage(groupDominator)
                                << ") is before the last "
                                   "use of the previous field ("
                                << lastField->maxStage.first << lastField->maxStage.second << ")");
                return std::nullopt;
            }

            // Check that the initialization point cannot reach the units using the last field.
            if (!ARAspill) {
                const IR::BFN::Unit *groupDominatorUnit = groupDominator->to<IR::BFN::Unit>();
                auto reach_condition = canFUnitsReachGUnits({groupDominatorUnit}, lastField->units,
                                                            domTree.getFlowGraph());
                if (reach_condition.size() > 0) {
                    bool ignoreReach = ignoreReachCondition(info, *lastField, reach_condition);
                    if (!ignoreReach) {
                        LOG_FEATURE("alloc_progress", 5,
                                    TAB2
                                        "Cannot find initialization point"
                                        " because group dominator can reach one of the uses of the "
                                        "last field "
                                        << lastField->field);
                        return std::nullopt;
                    }
                }

                // Check that units using the last field can reach the initialization point.
                goToNextDominator = !groupDominatorBeforeFirstUseCurrentField;

                if (!goToNextDominator) {
                    for (auto *u : lastField->units) {
                        auto reach_condition =
                            canFUnitsReachGUnits({u}, {groupDominatorUnit}, domTree.getFlowGraph());
                        if (reach_condition.size() == 0) {
                            LOG_DEBUG2(TAB2 "Use unit " << DBPrint::Brief << u
                                                        << " of previous "
                                                           "field "
                                                        << lastField->field
                                                        << " cannot reach "
                                                           "initialization point "
                                                        << DBPrint::Brief << groupDominatorUnit);
                            goToNextDominator = true;
                            break;
                        }
                    }
                } else {
                    LOG_DEBUG3(TAB2 "Cannot initialize current field before its first use.");
                }

                // If we are initializing the current field, then make sure that the initialization
                // point does not cause a lengthening of the critical path.
                if (!goToNextDominator && initializeCurrentField) {
                    for (auto *u : f_nodes) {
                        const auto *t = u->to<IR::MAU::Table>();
                        if (!t) continue;
                        if (increasesDependenceCriticalPath(t, groupDominator)) {
                            LOG_DEBUG2(TAB2 "Cannot initialize at "
                                       << groupDominator->name
                                       << " because it would increase the critical path through "
                                          "the dependency graph (via use at "
                                       << DBPrint::Brief << u);
                            goToNextDominator = true;
                            break;
                        }
                    }
                }

                if (!goToNextDominator && doNotInitTables.count(groupDominator)) {
                    LOG_DEBUG2(TAB2 "Table "
                               << groupDominator->name
                               << " requires more than one "
                                  "stage. Therefore, we will not initialize at that table.");
                    goToNextDominator = true;
                }

                // Mostly the same check as the one before except that it is based on previous
                // Table Alloc round result.
                if (!goToNextDominator && tableAlloc.stage(groupDominator, true).size() > 1) {
                    LOG2(TAB2 "Table " << groupDominator->name
                                       << " requires more than one stage."
                                          " Therefore, we will not initialize at that table.");
                    goToNextDominator = true;
                }
            }

            // Only find initialization point if this group dominator is the right candidate.
            if (ARAspill || !goToNextDominator) {
                if (ARAspill) {
                    LOG_DEBUG2(TAB2 "Trying to initialize using ARA");
                } else {
                    LOG_DEBUG2(TAB2 "Trying to initialize at table "
                               << groupDominator->name << " (Stage " << dg.min_stage(groupDominator)
                               << ")");
                }

                auto darkInitPoints = getInitPointsForTable(
                    group, c, groupDominator, *lastField, info, rv, movePreviousToDark,
                    initializeCurrentField, initializeFromDark, alloc, ARAspill);
                if (!groupDominatorBeforeFirstUseCurrentField) {
                    // TODO: Relax by accounting for valid uses directly from dark containers.
                    LOG_DEBUG3(TAB2 "Cannot initialize current field before its first use.");
                } else if (!darkInitPoints) {
                    if (ARAspill) {
                        LOG_FEATURE("alloc_progress", 5,
                                    "Did not find any initialization points"
                                    " using ARA.");
                        return std::nullopt;
                    } else {
                        LOG_FEATURE("alloc_progress", 5,
                                    TAB2
                                    "Did not get an initialization "
                                    "dominator; need to move up in the flow graph.");
                    }
                } else if (darkInitPoints) {
                    // Found initializations. Now set up the return vector accordingly.
                    LOG_DEBUG3(TAB2 << darkInitPoints->size() << " initializations found");
                    for (auto init : *darkInitPoints) {
                        LOG_DEBUG3(TAB2 "Adding to return vector: " << init);
                        rv.push_back(init);
                        firstDarkInitMaxStage =
                            init.getDestinationSlice().getEarliestLiveness().first;
                    }
                    break;
                }
            }
            auto newGroupDominator =
                domTree.getNonGatewayImmediateDominator(groupDominator, groupDominator->thread());
            if (!newGroupDominator) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Could not find an initialization points "
                                 "for previous field "
                                << lastField->field);
                return std::nullopt;
            } else if (*newGroupDominator == groupDominator) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB2 "Reached the beginning of the flow graph."
                                 " Cannot initialize previous field "
                                << lastField->field);
                return std::nullopt;
            } else {
                groupDominator = *newGroupDominator;
                LOG_DEBUG2(TAB2 "Setting new group dominator to " << DBPrint::Brief
                                                                  << groupDominator);
            }

            BUG_CHECK(!ARAspill, "Reached nextDominator while ARAspill ...?");
        }  // while (groupDominator)

        if (idx == 1 && firstDarkInitEntry != nullptr) {
            LOG_DEBUG3("Need to push the first dark init primitive corresponding to "
                       << *firstDarkInitEntry);
            if (movePreviousToDark) {
                LOG_DEBUG3("Live range must extend to the initialization point");
                firstDarkInitMaxStage =
                    ARAspill ? firstDarkInitMaxStage : dg.min_stage(groupDominator);

                firstDarkInitEntry->setDestinationLatestLiveness(
                    std::make_pair(firstDarkInitMaxStage, PHV::FieldUse(READ)));
                if (!ARAspill)
                    firstDarkInitEntry->addDestinationUnit(groupDominator->name,
                                                           PHV::FieldUse(READ));
                LOG_DEBUG3("New dark primitive: " << *firstDarkInitEntry);
                rv.insert(rv.begin(), *firstDarkInitEntry);
            }
        }

        // Add check here for critical path increase due to this movement.

        // Arrive at a common node for all these units to initialize the container c to 0, and to
        // move the existing contents of container c to a dark container.
        ++idx;
        lastField = &info;
    }

    return rv;
}

// Difference between LiveRangeShrinking and DarkOverlay is that live range shrinking initializes
// the current field, whereas DarkOverlay moves the previous field to dark and brings the current
// field into a normal container (2 initializations potentially).
std::optional<PHV::DarkInitMap> DarkLiveRange::getInitPointsForTable(
    const PHV::ContainerGroup &group, const PHV::Container &c, const IR::MAU::Table *t,
    const OrderedFieldInfo &lastField, const OrderedFieldInfo &currentField,
    PHV::DarkInitMap &initMap, bool moveLastFieldToDark, bool initializeCurrentField,
    bool initializeFromDark, const PHV::Transaction &alloc, bool useARA) const {
    PHV::DarkInitMap rv;

    bool lastMutexSatisfied = true;
    bool currentMutexSatisfied = true;

    if (!useARA) {
        // If the last field is to be moved into a dark container, make sure that the uses of that
        // slice (for that live range) is not mutually exclusive with the table t, where the move
        // is supposed to happen.
        if (moveLastFieldToDark) lastMutexSatisfied = mutexSatisfied(lastField, t);

        // If the current field is to be initialized, make sure that the uses of that slice (for the
        // corresponding live range) is not mutually exclusive with the table t, where the
        // initialization is to be performed.
        if (initializeCurrentField) currentMutexSatisfied = mutexSatisfied(currentField, t);

        if (!lastMutexSatisfied || !currentMutexSatisfied) return std::nullopt;
    }

    PHV::DarkInitEntry *prevSlice = nullptr;
    int cur_min_stage = currentField.get_min_stage(dg);
    BUG_CHECK(cur_min_stage >= 0, "Found invalid min_stage for slice %1%", currentField.field);

    // In the future we may want to use regular actions for spills to dark
    //       So we will need to add logic to determine useARA

    std::optional<PHV::DarkInitEntry *> darkFieldInit = std::nullopt;
    std::optional<PHV::DarkInitEntry *> currentFieldInit = std::nullopt;

    // Check if moving lastField to dark container (if required) is possible.
    if (moveLastFieldToDark) {
        darkFieldInit =
            getInitForLastFieldToDark(c, group, t, lastField, alloc, currentField, useARA);
        if (!darkFieldInit) return std::nullopt;
        LOG_DEBUG3(TAB3 "A. Creating" << (useARA ? " ARA " : " non-ARA ")
                                      << " dark init "
                                         "primitive for moving last field to dark : "
                                      << **darkFieldInit);

        if (useARA) {
            // Update prior and post table constraints for injection
            // during AddDarkInitialization pass
            (*darkFieldInit)->addPriorUnits(lastField.units);
            (*darkFieldInit)->addPostUnits(currentField.units);

            // Tag spill to dark initialization as AlwaysRunAction
            (*darkFieldInit)->setAlwaysRunInit();
        }

        auto srcSlice = (*darkFieldInit)->getSourceSlice();
        if (srcSlice) {
            if (initMap.size() > 0) {
                PHV::DarkInitEntry *lastSlice = &initMap[initMap.size() - 1];
                PHV::StageAndAccess newLatestStage = std::make_pair(
                    (*darkFieldInit)->getDestinationSlice().getEarliestLiveness().first,
                    PHV::FieldUse(READ));
                lastSlice->setDestinationLatestLiveness(newLatestStage);
                lastSlice->addRefs(srcSlice->getRefs());
                LOG_DEBUG3(TAB3 "Updating latest liveness and refs of " << *lastSlice);

                // Also update the lifetime of prior/post prims related to the source slice
                for (auto *prim : lastSlice->getInitPrimitive().getARApostPrims()) {
                    LOG_DEBUG4(
                        TAB2 "Updating latest liveness for: " << prim->getDestinationSlice());
                    LOG_DEBUG4(TAB3 "to " << newLatestStage);
                    prim->setDestinationLatestLiveness(newLatestStage);
                }
                for (auto *prim : lastSlice->getInitPrimitive().getARApostPrims()) {
                    LOG_DEBUG4("After update: " << prim->getDestinationSlice());
                }

                for (auto *prim : lastSlice->getInitPrimitive().getARApriorPrims()) {
                    LOG_DEBUG4(
                        TAB2 "Updating latest liveness for: " << prim->getDestinationSlice());
                    LOG_DEBUG4(TAB3 "to " << newLatestStage);
                    prim->setDestinationLatestLiveness(newLatestStage);
                }
                for (auto *prim : lastSlice->getInitPrimitive().getARApriorPrims()) {
                    LOG_DEBUG4("After update: " << prim->getDestinationSlice());
                }
            }
        }
    } else {
        // Reset the liveness for this slice based on the initialization point.
        if (initMap.size() > 0) {
            prevSlice = &initMap[initMap.size() - 1];
        } else {
            PHV::AllocSlice dest(lastField.field);
            // Here we can use the dominator t since moveLastFieldToDark==false
            dest.setLiveness(lastField.minStage,
                             std::make_pair(dg.min_stage(t), PHV::FieldUse(READ)));
            PHV::DarkInitEntry noInitDark(dest);
            noInitDark.setNop();
            LOG_DEBUG3(TAB3 "B. Creating dark init primitive: " << noInitDark);
            rv.push_back(noInitDark);
        }
    }

    // Check if there is an allocation for field currentField. If there is, then it must be in a
    // dark container, which can then be moved back into this container (c).
    if (initializeCurrentField && initializeFromDark) {
        currentFieldInit = getInitForCurrentFieldFromDark(c, t, currentField, initMap, alloc);
    } else if (initializeCurrentField && !initializeFromDark) {
        currentFieldInit =
            getInitForCurrentFieldWithZero(c, t, currentField, alloc, darkFieldInit, useARA);

        // Add a dependency between the previous slice's move-to-dark and the
        // current slice's init-to-zero primitives
        if (darkFieldInit && useARA) {
            BUG_CHECK(currentFieldInit,
                      "Last field moved to dark but no zero initialization for current field");

            (*currentFieldInit)->setAlwaysRunInit();
            // Update prior and post table constraints for injection
            // during AddDarkInitialization pass
            (*currentFieldInit)->addPriorUnits(lastField.units);
            (*currentFieldInit)->addPostUnits(currentField.units);

            LOG_DEBUG4(TAB1 "Adding Prior DarkInitEntry " << **darkFieldInit << "\n" TAB2 " to "
                                                          << **currentFieldInit);
            (*currentFieldInit)->addPriorPrims(*darkFieldInit);
        }
    }

    if (!currentFieldInit) return std::nullopt;

    if (darkFieldInit) {
        if (useARA) {
            LOG_DEBUG4(TAB1 "Adding Post DarkInitEntry " << **currentFieldInit << "\n " TAB2 " to "
                                                         << **darkFieldInit);
            (*darkFieldInit)->addPostPrims(*currentFieldInit);
        }

        rv.push_back(**darkFieldInit);
    }

    rv.push_back(**currentFieldInit);

    if (prevSlice != nullptr) {
        BUG_CHECK(t, "Dominator node is not set");
        PHV::StageAndAccess newLatestStage = std::make_pair(dg.min_stage(t), PHV::FieldUse(READ));
        prevSlice->setDestinationLatestLiveness(newLatestStage);
        LOG_DEBUG5(TAB2 "Setting latest liveness for previous slice : " << *prevSlice);
    }

    return rv;
}

std::optional<PHV::ActionSet> DarkLiveRange::getInitActions(const PHV::Container &c,
                                                            const OrderedFieldInfo &field,
                                                            const IR::MAU::Table *t,
                                                            const PHV::Transaction &alloc) const {
    PHV::ActionSet moveActions;
    const PHV::Field *f = field.field.field();
    for (const auto *act : tablesToActions.getActionsForTable(t)) {
        // If field is already written in this action, do not initialize here.
        //  This should be updated to be done at slice granularity
        if (actionConstraints.written_in(field.field, act)) {
            LOG5("\tA. Field " << field.field << " already written in action " << act->name
                               << " of table " << t->name);
            // continue;
        }
        if (cannotInitInAction(c, act, alloc)) {
            LOG_FEATURE(
                "alloc_progress", 5,
                TAB3 "Cannot init " << field.field << " in do not init action " << act->name);
            return std::nullopt;
        }
        if (actionConstraints.written_in(field.field, act)) {
            LOG5("\tB. Field " << field.field << " already written in action " << act->name
                               << " of table " << t->name);
            continue;
        }
        auto &actionReads = actionConstraints.actionReadsSlices(act);
        auto actionWrites = actionConstraints.actionWritesSlices(act);
        auto inits = alloc.getMetadataInits(act);
        for (const auto *g : inits) {
            LOG_DEBUG5(TAB3 "Noting down initialization of " << g->name << " for action "
                                                             << act->name);
            actionWrites.insert(PHV::FieldSlice(g, StartLen(0, g->size)));
        }
        // If any of the fields read or written by the action are mutually exclusive with the field
        // to be initialized, then do not initialize the field in this table.
        for (const auto &g : actionReads) {
            if (phv.isFieldMutex(f, g.field())) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB4 "Ignoring table "
                                << t->name << " as a node for moving " << field.field
                                << " because it is mutually exclusive with slice " << g
                                << " read by action " << act->name);
                return std::nullopt;
            }
        }
        for (const auto &g : actionWrites) {
            if (phv.isFieldMutex(f, g.field())) {
                LOG_FEATURE("alloc_progress", 5,
                            TAB4 "Ignoring table "
                                << t->name << " as a node for moving " << field.field
                                << " because it is mutually exclusive with slice " << g
                                << " written by action " << act->name);
                return std::nullopt;
            }
        }
        moveActions.insert(act);
    }
    if (moveActions.size() > 0) {
        LOG_DEBUG5(TAB4 "Initialization actions:");
        for (const auto *act : moveActions) LOG_DEBUG5(TAB4 << act->name);
    }
    return moveActions;
}

std::optional<PHV::DarkInitEntry *> DarkLiveRange::getInitForLastFieldToDark(
    const PHV::Container &c, const PHV::ContainerGroup &group, const IR::MAU::Table *t,
    const OrderedFieldInfo &prvField, const PHV::Transaction &alloc,
    const OrderedFieldInfo &curField, bool useARA) const {
    // Find out all the actions in table t, where we need to insert moves into the dark container.
    // DXm[a...b] = Xn[a...b]
    // Only need to do this for non ARA move-to-dark inits
    std::optional<PHV::ActionSet> moveActions;
    if (!useARA) {
        moveActions = getInitActions(c, prvField, t, alloc);
        if (!moveActions) return std::nullopt;
    } else {
        if ((prvField.maxStage.first >= (curField.minStage.first)) ||
            ((prvField.maxStage.first == (curField.minStage.first - 1)) &&
             prvField.maxStage.second.isWrite())) {
            LOG_FEATURE("alloc_progress", 5,
                        TAB3 "Cannot do ARA spill due to slice liveranges: "
                             "previousMax("
                            << prvField.maxStage << ") currentMin(" << curField.minStage << ")");
            return std::nullopt;
        }
    }

    auto darkCandidates = group.getAllContainersOfKind(PHV::Kind::dark);
    LOG_DEBUG5(TAB4 "Overlay container: " << c);
    for (auto dark : darkCandidates) LOG_DEBUG5(TAB4 "Candidate dark container: " << dark);

    // Get best dark container to move the field into.
    const PHV::Container darkCandidate = getBestDarkContainer(darkCandidates, prvField, alloc);
    if (darkCandidate == PHV::Container()) {
        LOG_FEATURE(
            "alloc_progress", 5,
            TAB3 "Could not find a dark container to move field " << prvField.field << " into");
        return std::nullopt;
    }
    LOG_DEBUG5(TAB3 "Best container for dark: " << darkCandidate);

    int minDarkStage =
        prvField.maxStage.second.isRead() ? prvField.maxStage.first : prvField.maxStage.first + 1;

    PHV::AllocSlice srcSlice(prvField.field);
    if (useARA)
        srcSlice.setLiveness(prvField.minStage, std::make_pair(minDarkStage, PHV::FieldUse(READ)));
    else
        srcSlice.setLiveness(prvField.minStage,
                             std::make_pair(dg.min_stage(t), PHV::FieldUse(READ)));
    LOG_DEBUG5(TAB4 "Created source slice " << srcSlice);

    PHV::AllocSlice dstSlice(prvField.field.field(), darkCandidate, prvField.field.field_slice(),
                             prvField.field.container_slice());
    // Set maximum liveness for this slice.
    int maxDarkStage = curField.maxStage.second == PHV::FieldUse(READ)
                           ? curField.maxStage.first
                           : curField.maxStage.first + 1;
    dstSlice.setLiveness(std::make_pair(srcSlice.getLatestLiveness().first, PHV::FieldUse(WRITE)),
                         std::make_pair(maxDarkStage, PHV::FieldUse(READ)));
    LOG_DEBUG5(TAB4 "Created destination slice " << dstSlice);

    // Use actions only for non-ARA primitives
    if (useARA) {
        // Don't have an ARA table yet to add to the slice's units
        return new PHV::DarkInitEntry(dstSlice, srcSlice);
    } else {
        LOG_DEBUG5("\t  A. Add unit " << t->name << " to slice " << dstSlice);
        dstSlice.addRef(t->name, PHV::FieldUse(WRITE));
        srcSlice.addRef(t->name, PHV::FieldUse(READ));
        return new PHV::DarkInitEntry(dstSlice, srcSlice, *moveActions);
    }
}

std::optional<PHV::DarkInitEntry *> DarkLiveRange::getInitForCurrentFieldWithZero(
    const PHV::Container &c, const IR::MAU::Table *t, const OrderedFieldInfo &field,
    const PHV::Transaction &alloc, std::optional<PHV::DarkInitEntry *> drkInit, bool useARA) const {
    // TODO:
    // Check if any pack conflicts are violated.
    // Initializing at table t requires that there is a dependence now from the previous uses of
    // prevField to table t.

    // TODO: Check if field is always written first on all paths.

    // Find out all actions in table t, where we need to initialize @field to 0 in container @c.
    // c[a...b] = 0
    // Use initAction for non-ARA primitives
    std::optional<PHV::ActionSet> initActions;
    if (!useARA) {
        initActions = getInitActions(c, field, t, alloc);
        if (!initActions) return std::nullopt;
    }

    PHV::AllocSlice dstSlice(field.field);
    int earlyLRstg =
        ((useARA && drkInit) ? (*drkInit)->getDestinationSlice().getEarliestLiveness().first
                             : dg.min_stage(t));
    dstSlice.setLiveness(std::make_pair(earlyLRstg, PHV::FieldUse(WRITE)), field.maxStage);
    LOG_DEBUG5(TAB4 "Created destination slice " << dstSlice);
    // Use initAction for non-ARA primitives
    if (useARA) {
        if (LOGGING(5)) {
            // Don't have an ARA table yet to add to the slice's units.
            LOG_DEBUG5("\t  B. Adding units to slice " << dstSlice);
            for (auto entry : field.field.getRefs())
                LOG_DEBUG5("\t\t " << entry.first << " (" << entry.second << ")");
        }
        dstSlice.addRefs(field.field.getRefs());
        return new PHV::DarkInitEntry(dstSlice, PHV::ActionSet());
    } else {
        BUG_CHECK((initActions->size() > 0), "No actions found to zero init dark overlay!");
        LOG_DEBUG5("\t  B. Add unit " << t->name << " to slice " << dstSlice);
        dstSlice.addRef(t->name, PHV::FieldUse(WRITE));
        if (LOGGING(5)) {
            LOG_DEBUG5("\t  C. Adding units to slice " << dstSlice);
            for (auto entry : field.field.getRefs())
                LOG_DEBUG5("\t\t " << entry.first << " (" << entry.second << ")");
        }
        dstSlice.addRefs(field.field.getRefs());
        return new PHV::DarkInitEntry(dstSlice, *initActions);
    }
}

std::optional<PHV::DarkInitEntry> DarkLiveRange::generateInitForLastStageAlwaysInit(
    const OrderedFieldInfo &field, const OrderedFieldInfo *prvField,
    PHV::DarkInitMap &darkInitMap) const {
    PHV::AllocSlice dstSlice(field.field);
    int fromDarkStage = prvField->maxStage.second == PHV::FieldUse(READ)
                            ? prvField->maxStage.first
                            : (prvField->maxStage.first + 1);
    dstSlice.setLiveness(std::make_pair(fromDarkStage, PHV::FieldUse(WRITE)), field.maxStage);
    if (LOGGING(5)) {
        LOG_DEBUG5("\t  D. Adding units to slice " << dstSlice << " (Clear previous units)");
        for (auto entry : field.field.getRefs())
            LOG_DEBUG5("\t\t " << entry.first << " (" << entry.second << ")");
    }
    dstSlice.addRefs(field.field.getRefs(), /*replace*/ true);
    PHV::DarkInitEntry rv(dstSlice);
    for (auto it = darkInitMap.rbegin(); it != darkInitMap.rend(); ++it) {
        PHV::AllocSlice dest = it->getDestinationSlice();
        bool found = (dest.field() == field.field.field() &&
                      dest.field_slice() == field.field.field_slice() &&
                      dest.container_slice() == field.field.container_slice() &&
                      dest.width() == field.field.width());
        if (found) {
            dest.setLatestLiveness(std::make_pair(fromDarkStage, PHV::FieldUse(READ)));
            it->setDestinationLatestLiveness(std::make_pair(fromDarkStage, PHV::FieldUse(READ)));
            rv.addSource(dest);
            rv.setLastStageAlwaysInit();
            LOG_DEBUG3(TAB3 "Adding initialization from dark in last stage: " << rv);
            rv.addPriorUnits(prvField->units);
            return rv;
        }
    }
    BUG("Did not find allocation for slice %1% in a dark container", field.field);
    return std::nullopt;
}

std::optional<PHV::DarkInitEntry> DarkLiveRange::generateARAzeroInit(
    const OrderedFieldInfo &field, const OrderedFieldInfo *prvField, const PHV::Transaction &alloc,
    bool onlyDeparserUse, bool onlyReadCandidates) const {
    PHV::AllocSlice dstSlice(field.field);
    // Initially set the starting stage of the current liverange to the first ref stage
    int initStage = field.minStage.first;
    // If we need to initialize the current field we need to adjust the earliest liverange
    if (!onlyReadCandidates) {
        // if slice is already allocated and has later starting liverange
        if (alloc.slices(dstSlice.container(), dstSlice.field_slice()).count(dstSlice) &&
            (dstSlice.getEarliestLiveness().first > prvField->maxStage.first))
            initStage = dstSlice.getEarliestLiveness().first;
        else
            initStage = prvField->maxStage.second == PHV::FieldUse(READ)
                            ? prvField->maxStage.first
                            : (prvField->maxStage.first + 1);
    }

    dstSlice.setLiveness(std::make_pair(initStage, (onlyReadCandidates ? PHV::FieldUse(READ)
                                                                       : PHV::FieldUse(WRITE))),
                         field.maxStage);
    dstSlice.addRefs(field.field.getRefs());

    PHV::DarkInitEntry rv(dstSlice, PHV::ActionSet());
    // Handle padding fields
    if (field.field.field()->padding || onlyReadCandidates) {
        rv.setNop();
        //  This is not actually required for overlaid fields that are only read
        // rv.addPriorUnits(prvField->units);
        LOG_DEBUG3(TAB5 "Adding NOP initialization for non-written slice: " << rv);
        return rv;
    }

    // Check if we can inject ARA without pushing uses of @field later
    if ((field.minStage.first - initStage) < 1) {
        LOG_FEATURE("alloc_progress", 5,
                    TAB2 "No stage for ARA zero-init. Prev maxStage:"
                        << prvField->maxStage << "  Cur minStage:" << field.minStage);
        return std::nullopt;
    }

    if (onlyDeparserUse) {
        rv.setLastStageAlwaysInit();
    } else {
        rv.setAlwaysRunInit();
        rv.addPostUnits(field.units);
    }

    rv.addPriorUnits(prvField->units);
    LOG_DEBUG3(TAB5 "Adding ARA zero initialization: " << rv);
    return rv;
}

std::optional<PHV::DarkInitEntry *> DarkLiveRange::getInitForCurrentFieldFromDark(
    const PHV::Container &c, const IR::MAU::Table *t, const OrderedFieldInfo &field,
    PHV::DarkInitMap &initMap, const PHV::Transaction &alloc) const {
    auto initActions = getInitActions(c, field, t, alloc);
    if (!initActions) return std::nullopt;
    // Start from the last element in the initMap vector, and find the latest AllocSlice that
    // matches the field slice for the current field.
    PHV::AllocSlice dstSlice(field.field);
    dstSlice.setLiveness(std::make_pair(dg.min_stage(t), PHV::FieldUse(WRITE)), field.maxStage);
    // Add the dominator table used for the move from dark into units
    LOG_DEBUG5("\t  C. Add unit " << t->name << " to slice " << dstSlice);
    dstSlice.addRef(t->name, PHV::FieldUse(WRITE));
    // Add the remaining use/def tables that reference this OrderedFieldInfo into units
    if (LOGGING(5)) {
        LOG_DEBUG5("\t  E. Adding units to slice " << dstSlice);
        for (auto entry : field.field.getRefs())
            LOG_DEBUG5("\t\t " << entry.first << " (" << entry.second << ")");
    }
    dstSlice.addRefs(field.field.getRefs());
    auto *rv = new PHV::DarkInitEntry(dstSlice, *initActions);
    for (auto it = initMap.rbegin(); it != initMap.rend(); ++it) {
        PHV::AllocSlice dest = it->getDestinationSlice();
        bool found = (dest.field() == field.field.field() &&
                      dest.field_slice() == field.field.field_slice() &&
                      dest.container_slice() == field.field.container_slice() &&
                      dest.width() == field.field.width());
        if (found) {
            PHV::StageAndAccess newReadStage = std::make_pair(dg.min_stage(t), PHV::FieldUse(READ));
            PHV::StageAndAccess newWriteStage =
                std::make_pair(dg.min_stage(t), PHV::FieldUse(WRITE));
            // Current initialization point becomes start of liveness of new slice and end of
            // liveness of source slice.
            it->setDestinationLatestLiveness(newReadStage);
            it->addDestinationUnit(t->name, PHV::FieldUse(READ));
            rv->setDestinationEarliestLiveness(newWriteStage);
            dest.setLatestLiveness(newReadStage);
            dest.addRef(t->name, PHV::FieldUse(READ));
            rv->addSource(dest);
            LOG_DEBUG3(TAB3 "Adding initialization from dark: " << *rv);
            return rv;
        }
    }
    BUG("Did not find allocation for slice %1% in a dark container", field.field);
    return std::nullopt;
}

const PHV::Container DarkLiveRange::getBestDarkContainer(
    const ordered_set<PHV::Container> &darkContainers, const OrderedFieldInfo &nextField,
    const PHV::Transaction &alloc) const {
    // PHV::Container bestContainer;
    // int bestScore = -1;
    // This is the number of stages we see in the table dependency graph. The score is this number
    // minus the number of stages allocated already to the dark container. Smaller scores are
    // better.
    // const unsigned maxStages = dg.max_min_stage + 1;
    auto fieldGress = nextField.field.field()->gress;
    // Get the best dark container for the purpose.
    for (auto c : darkContainers) {
        auto containerGress = alloc.gress(c);
        // Change to checking if dark containers are available at the particular stage range.
        if (containerGress) continue;
        auto parserGroupGress = alloc.parserGroupGress(c);
        auto deparserGroupGress = alloc.deparserGroupGress(c);
        bool containerGressOk =
            (!containerGress) || (containerGress && *containerGress == fieldGress);
        bool parserGressOk =
            (!parserGroupGress) || (parserGroupGress && *parserGroupGress == fieldGress);
        bool deparserGressOk =
            (!deparserGroupGress) || (deparserGroupGress && *deparserGroupGress == fieldGress);
        if (containerGressOk && parserGressOk && deparserGressOk) return c;
    }
    return PHV::Container();
}

const IR::MAU::Table *DarkLiveRange::getGroupDominator(const PHV::Field *f,
                                                       const PHV::UnitSet &f_units,
                                                       gress_t gress) const {
    LOG_DEBUG1(TAB2 "getGroupDominator : " << f << " for gress: " << gress);
    ordered_map<const IR::MAU::Table *, const IR::BFN::Unit *> tablesToUnits;
    for (const auto *u : f_units) {
        if (u->is<IR::BFN::Deparser>()) {
            auto t = domTree.getNonGatewayImmediateDominator(nullptr, gress);
            if (!t) {
                LOG_DEBUG2(TAB3 "No table dominators for use unit: " << DBPrint::Brief << u);
                return nullptr;
            }
            tablesToUnits[*t] = (*t)->to<IR::BFN::Unit>();
            LOG_DEBUG2(TAB3 "Adding Table - Unit : " << DBPrint::Brief << u);
            continue;
        }
        const auto *t = u->to<IR::MAU::Table>();
        BUG_CHECK(t, "Non-deparser non-table use found.");
        tablesToUnits[t] = u;
        LOG_DEBUG2(TAB3 "Adding Table - Unit : " << DBPrint::Brief << u);
    }
    if (tablesToUnits.size() == 0) return nullptr;
    ordered_set<const IR::MAU::Table *> tables;
    if (tablesToUnits.size() == 1) {
        auto &kv = *(tablesToUnits.begin());
        tables.insert(kv.first);
        LOG_DEBUG2(TAB3 "Insert Table : " << kv.first->name << " at gress " << kv.first->gress);
        if (defuse.hasUseAt(f, kv.second)) return domTree.getNonGatewayGroupDominator(tables);
        return kv.first;
    }
    for (auto &kv : tablesToUnits) tables.insert(kv.first);
    return domTree.getNonGatewayGroupDominator(tables);
}

bool DarkLiveRange::mustMoveToDark(const OrderedFieldInfo &field,
                                   const OrderedFieldSummary &fieldsInOrder) const {
    bool foundOriginalFieldInfo = false;
    for (auto &info : fieldsInOrder) {
        if (info == field) {
            foundOriginalFieldInfo = true;
            continue;
        }
        if (!foundOriginalFieldInfo) continue;
        // These are field slices after @field.
        if (info.field == field.field) return true;
    }
    return false;
}

bool DarkLiveRange::mustInitializeCurrentField(const OrderedFieldInfo &field,
                                               const PHV::UnitSet &fieldUses) const {
    ordered_set<const IR::MAU::Table *> tableUses;
    for (const auto *u : fieldUses) {
        if (u->is<IR::BFN::Deparser>()) continue;
        const IR::MAU::Table *t = u->to<IR::MAU::Table>();
        BUG_CHECK(t, "Field use unit %1% cannot be a non-table entity", u);
        tableUses.insert(t);
    }
    for (const auto *t : tableUses) {
        for (const auto *act : tablesToActions.getActionsForTable(t)) {
            // FIXME: Add metadata initialization points here.
            if (!actionConstraints.written_in(field.field, act)) {
                LOG_DEBUG3(TAB2 << field.field << " not written in action " << act->name
                                << " in table " << t->name);
                return true;
            } else {
                LOG_DEBUG3(TAB2 << field.field << " is written in action " << act->name
                                << " in table " << t->name);
            }
        }
    }
    return false;
}

bool DarkLiveRange::mustInitializeFromDarkContainer(
    const OrderedFieldInfo &field, const OrderedFieldSummary &fieldsInOrder) const {
    // If the slice represented by @field is found earlier in @fieldsInOrder, then it means that
    // field was already live earlier in the pipeline, and therefore, it will have been moved to a
    // dark container earlier.
    bool foundFieldUseBeforeOriginalInfo = false;
    for (auto &info : fieldsInOrder) {
        if (info == field) return foundFieldUseBeforeOriginalInfo;
        if (info.field == field.field) foundFieldUseBeforeOriginalInfo = true;
    }
    BUG("We should never reach this point. How did we pass a slice info object not within "
        "fieldsInOrder?");
    return false;
}

bool DarkLiveRange::cannotInitInAction(const PHV::Container &c, const IR::MAU::Action *action,
                                       const PHV::Transaction &alloc) const {
    // If the PHVs in this action are already unaligned, then we cannot add initialization in this
    // action.
    if (actionConstraints.cannot_initialize(c, action, alloc)) {
        LOG_DEBUG4(TAB3
                   "Action analysis indicates a pre-existing write using PHV/action "
                   "data/non-zero const to container "
                   << c << " in " << action->name << ". Cannot initialize here.");
        return true;
    }
    return doNotInitActions.count(action);
}

bool DarkLiveRange::mutexSatisfied(const OrderedFieldInfo &info, const IR::MAU::Table *t) const {
    ordered_set<const IR::MAU::Table *> tableUses;
    for (const auto *u : info.units)
        if (u->is<IR::MAU::Table>()) tableUses.insert(u->to<IR::MAU::Table>());
    for (const auto *tbl : tableUses) {
        if (tableMutex(t, tbl)) {
            LOG_DEBUG4(TAB3 "Ignoring table " << t->name
                                              << " beause it is mutually exclusive "
                                                 "with use table "
                                              << tbl->name << " of field " << info.field);
            return false;
        }
    }
    return true;
}

cstring DarkLiveRangeMap::printDarkLiveRanges() const {
    std::stringstream ss;
    auto numStages = DEPARSER;
    const int PARSER = -1;
    ss << std::endl << "Uses for fields to determine dark overlay potential:" << std::endl;
    ss << " *** LIVERANGE LEGEND - W/R: Dark compatible write/read   w/r: Dark incompatible "
          "wrote/read"
       << std::endl;
    std::vector<std::string> headers;
    headers.push_back("Field");
    headers.push_back("Bit Size");
    headers.push_back("P");
    for (int i = 0; i < numStages; i++) headers.push_back(std::to_string(i));
    headers.push_back("D");
    TablePrinter tp(ss, headers, TablePrinter::Align::LEFT);
    for (auto entry : livemap) {
        std::vector<std::string> row;
        row.push_back(std::string(entry.first->name));
        row.push_back(std::to_string(entry.first->size));
        PHV::FieldUse use_type;
        if (entry.second.count(std::make_pair(PARSER, PHV::FieldUse(WRITE))))
            use_type |= PHV::FieldUse(WRITE);
        if (entry.second.count(std::make_pair(PARSER, PHV::FieldUse(READ))))
            use_type |= PHV::FieldUse(READ);
        row.push_back(std::string(use_type.toString()));
        for (int i = 0; i <= DEPARSER; i++) {
            PHV::FieldUse use_type;
            unsigned dark = 0;
            if (entry.second.count(std::make_pair(i, PHV::FieldUse(READ)))) {
                use_type |= PHV::FieldUse(READ);
                if (entry.second.at(std::make_pair(i, PHV::FieldUse(READ))).second) dark |= 1;
            }
            if (entry.second.count(std::make_pair(i, PHV::FieldUse(WRITE)))) {
                use_type |= PHV::FieldUse(WRITE);
                if (entry.second.at(std::make_pair(i, PHV::FieldUse(WRITE))).second) dark |= 2;
            }
            row.push_back(std::string(use_type.toString(dark)));
        }
        tp.addRow(row);
    }
    tp.print();
    return ss.str();
}

std::optional<PHV::StageAndAccess> DarkLiveRangeMap::getEarliestAccess(const PHV::Field *f) const {
    if (!count(f)) {
        return std::nullopt;
    }
    const auto &keys = Keys(at(f));
    auto min = std::min_element(keys.begin(), keys.end());
    return min == keys.end() ? std::nullopt : std::optional<PHV::StageAndAccess>(*min);
}

std::optional<PHV::StageAndAccess> DarkLiveRangeMap::getLatestAccess(const PHV::Field *f) const {
    if (!count(f)) {
        return std::nullopt;
    }
    const auto &keys = Keys(at(f));
    auto max = std::max_element(keys.begin(), keys.end());
    return max == keys.end() ? std::nullopt : std::optional<PHV::StageAndAccess>(*max);
}

bool DarkOverlay::suitableForDarkOverlay(const PHV::AllocSlice &slice) const {
    if (slice.field()->is_solitary() && (slice.container().size() - slice.width() > 7))
        return false;
    return true;
}

DarkOverlay::DarkOverlay(PhvInfo &p, const ClotInfo &c, const DependencyGraph &g, FieldDefUse &f,
                         const PHV::Pragmas &pragmas, const PhvUse &u,
                         const ActionPhvConstraints &actions, const BuildDominatorTree &d,
                         const MapTablesToActions &m, const MauBacktracker &a,
                         const NonMochaDarkFields &nmd)
    : initNode(p, c, g, f, pragmas, u, d, actions, a, tableMutex, m, nmd) {
    addPasses({&tableMutex, &initNode});
}
