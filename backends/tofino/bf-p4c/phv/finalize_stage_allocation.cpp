/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/phv/finalize_stage_allocation.h"

bool CalcMaxPhysicalStages::preorder(const IR::MAU::Table* tbl) {
    int stage = tbl->stage();
    if (stage + 1 > deparser_stage[tbl->gress])
        deparser_stage[tbl->gress] = stage + 1;
    return true;
}

void CalcMaxPhysicalStages::end_apply() {
    for (auto i : deparser_stage)
        if (i > dep_stage_overall)
            dep_stage_overall = i;
    LOG1("  Deparser denoted by physical stage " << dep_stage_overall);
}

void FinalizeStageAllocation::summarizeUseDefs(
        const PhvInfo& phv,
        const DependencyGraph& dg,
        const FieldDefUse::LocPairSet& refs,
        StageFieldEntry& stageToTables,
        bool& usedInParser,
        bool& usedInDeparser,
        bool usePhysicalStages) {
    for (auto& ref : refs) {
        if (ref.first->is<IR::BFN::Parser>() || ref.first->is<IR::BFN::ParserState>() ||
            ref.first->is<IR::BFN::GhostParser>()) {
            if (!ref.second->is<ImplicitParserInit>()) {
                usedInParser = true;
                LOG5("\tUsed in parser");
            }
        } else if (ref.first->is<IR::BFN::Deparser>()) {
            usedInDeparser = true;
            LOG5("\tUsed in deparser");
        } else if (ref.first->is<IR::MAU::Table>()) {
            auto* t = ref.first->to<IR::MAU::Table>();
            le_bitrange bits;
            auto* f = phv.field(ref.second, &bits);
            CHECK_NULL(f);
            //  The stage info stored into PhvInfo::table_to_min_stage is still not physical.
            // So this if else clause is meaningless because in both cases we use dg generated
            // stage info
            if (usePhysicalStages) {
                BUG_CHECK(t->global_id() || (t->is_always_run_action() && t->stage() >= 0),
                          "Table %1% is unallocated", t->name);
                const auto minStages = PhvInfo::minStages(t);
                for (auto stage : minStages) {
                    stageToTables[stage][t].insert(bits);
                    LOG5("\tUsed in table " << t->name << " (Stage " << stage << ") : " <<
                         f->name << "[" << bits.hi << ":" << bits.lo << "]");
                }
            } else {
                stageToTables[dg.min_stage(t)][t].insert(bits);
                LOG5("\tUsed in table " << t->name << " (DG Stage " << dg.min_stage(t) << ") : " <<
                     f->name << "[" << bits.hi << ":" << bits.lo << "]");
            }
        } else {
            BUG("Found a reference %s in unit %s that is not the parser, deparser, or table",
                    ref.second->toString(), ref.first->toString());
        }
    }
}

void UpdateFieldAllocation::updateAllocation(PHV::Field* f) {
    static PHV::FieldUse read(PHV::FieldUse::READ);
    static PHV::FieldUse write(PHV::FieldUse::WRITE);
    FinalizeStageAllocation::StageFieldEntry readTables;
    FinalizeStageAllocation::StageFieldEntry writeTables;
    bool usedInParser = false, usedInDeparser = false;

    LOG3("\tUpdating allocation for " << f);

    FinalizeStageAllocation::summarizeUseDefs(phv, dg, defuse.getAllDefs(f->id), writeTables,
            usedInParser, usedInDeparser);
    FinalizeStageAllocation::summarizeUseDefs(phv, dg, defuse.getAllUses(f->id), readTables,
            usedInParser, usedInDeparser);
    if (f->aliasSource != nullptr) {
        LOG5("\t  Summarizing usedefs for alias source: " << f->aliasSource->name);
        FinalizeStageAllocation::summarizeUseDefs(phv, dg,
                defuse.getAllDefs(f->aliasSource->id),
                writeTables, usedInParser, usedInDeparser);
        FinalizeStageAllocation::summarizeUseDefs(phv, dg,
                defuse.getAllUses(f->aliasSource->id), readTables, usedInParser,
                usedInDeparser);
    } else if (phv.getAliasMap().count(f)) {
        const PHV::Field* aliasDest = phv.getAliasMap().at(f);
        LOG5("\t  Summarizing usedefs for alias dest: " << aliasDest->name);
        FinalizeStageAllocation::summarizeUseDefs(phv, dg, defuse.getAllDefs(aliasDest->id),
                writeTables, usedInParser, usedInDeparser);
        FinalizeStageAllocation::summarizeUseDefs(phv, dg, defuse.getAllUses(aliasDest->id),
                readTables, usedInParser, usedInDeparser);
    }

    std::stringstream ss;
    ss << "\t" << f->id << ": " << f->name << std::endl;
    ss << "\t Written by tables:" << std::endl;
    for (auto& kv : writeTables) {
        ss << "\t  " << kv.first << " : ";
        for (auto& kv1 : kv.second) {
            ss << kv1.first->name << " ";
        }
        ss << std::endl;
    }
    ss << "\t Read by tables:" << std::endl;
    for (auto& kv : readTables) {
        ss << "\t  " << kv.first << " : ";
        for (auto& kv1 : kv.second)
            ss << kv1.first->name << " ";
        ss << std::endl;
    }
    LOG5("PHV Allocation stage info for field: \n" << ss.str());

    // Find the earliest alive slice per field bitrange and store it in minStageAccount
    ordered_map<le_bitrange, PHV::StageAndAccess> minStageAccount;
    ordered_set<le_bitrange> all_alloc_ranges;
    for (const auto& alloc : f->get_alloc()) {
        le_bitrange range = alloc.field_slice();
        if (!all_alloc_ranges.count(range)) {
            all_alloc_ranges.push_back(range);
        }
    }
    for (auto& alloc : f->get_alloc()) {
        le_bitrange alloc_range = alloc.field_slice();
        for (const auto& range : all_alloc_ranges) {
            if (alloc_range.contains(range)) {
                if (!minStageAccount.count(range)) {
                    minStageAccount[range] = alloc.getEarliestLiveness();
                } else {
                    auto candidate = minStageAccount.at(range);
                    if (candidate.first > alloc.getEarliestLiveness().first ||
                        (candidate.first == alloc.getEarliestLiveness().first &&
                        candidate.second > alloc.getEarliestLiveness().second))
                        minStageAccount[range] = alloc.getEarliestLiveness();
                }
            }
        }
    }

    int physDeparser = std::max(depStages.getDeparserStage(), Device::numStages());
    // Map minStage liverange to physical liverange and update each AllocSlice
    for (auto& alloc : f->get_alloc()) {
        if (parserMin == alloc.getEarliestLiveness() && deparserMax == alloc.getLatestLiveness()) {
            // Change max stage to deparser in the physical stage list.
            PHV::StageAndAccess max = std::make_pair(physDeparser, write);
            alloc.setLatestLiveness(max);
            alloc.setPhysicalDeparserStage(true);
            LOG5(ss.str() << "\tIgnoring field slice: " << alloc);
            continue;
        } else {
            LOG3("\t  Slice: " << alloc);
            LOG5("\t\tNot ignoring for parserMin " << parserMin.first << parserMin.second <<
                 " and deparserMax " << deparserMax.first << deparserMax.second);
            LOG5("\t\talloc min: " << alloc.getEarliestLiveness().first <<
                 alloc.getEarliestLiveness().second);
            LOG5("\t\talloc max: " << alloc.getLatestLiveness().first <<
                 alloc.getLatestLiveness().second);
        }
        bool includeParser = false;
        if (usedInParser) {
            if (alloc.getEarliestLiveness() == minStageAccount.at(alloc.field_slice())) {
                includeParser = true;
                LOG3("\t\tInclude parser use in this slice.");
            }
        }
        bool alwaysRunLastStageSlice = false;
        if (usedInDeparser) {
            LOG5("\t\t  used in deparser");
            LOG5("\t\t  Alloc: " << alloc);
            LOG5("\t\t  empty: " << alloc.getInitPrimitive().isEmpty() << ", always init: " <<
                 alloc.getInitPrimitive().mustInitInLastMAUStage() << ", shadow always init: " <<
                 alloc.getShadowAlwaysRun());
            if ((!alloc.getInitPrimitive().isEmpty() &&
                 (alloc.getInitPrimitive().mustInitInLastMAUStage())) ||
                alloc.getShadowAlwaysRun()) {
                alwaysRunLastStageSlice = true;
                LOG4("\t\tDetected slice for always run write-back from dark prior to deparsing");
            }
        }
        int minStageRead = (alloc.getEarliestLiveness().second == read) ?
                            alloc.getEarliestLiveness().first :
                            (alloc.getEarliestLiveness().first + 1);
        int maxStageRead = alloc.getLatestLiveness().first;
        int minStageWritten = alloc.getEarliestLiveness().first;
        int maxStageWritten = (alloc.getLatestLiveness().second == write) ?
                               alloc.getLatestLiveness().first :
                              (alloc.getLatestLiveness().first == 0 ? 0 :
                               alloc.getLatestLiveness().first - 1);
        LOG3("\t\tRead: [" << minStageRead << ", " << maxStageRead << "], Write: [" <<
             minStageWritten << ", " << maxStageWritten << "]");

        const int NOTSET = -2;
        int minPhysicalRead, maxPhysicalRead, minPhysicalWrite, maxPhysicalWrite;
        minPhysicalRead = maxPhysicalRead = minPhysicalWrite = maxPhysicalWrite = NOTSET;

        for (auto stage = minStageRead; stage <= maxStageRead; stage++) {
            LOG5("\t\t\tRead Stage: " << stage);
            if (readTables.count(stage)) {
                for (const auto& kv : readTables.at(stage)) {
                    bool foundFieldBits = std::any_of(kv.second.begin(), kv.second.end(),
                            [&](le_bitrange range) {
                        return alloc.field_slice().overlaps(range);
                    });
                    if (!foundFieldBits) continue;
                    int physStage = kv.first->stage();
                    LOG3("\t\t  Read table: " << kv.first->name << ", Phys stage: " << physStage);
                    bool foundTblRef = false;
                    for (auto refEntry : alloc.getRefs()) {
                        if (refEntry.first == kv.first->name) {
                            foundTblRef = true;
                            break;
                        }
                        if (!kv.first->gateway_name.isNullOrEmpty() &&
                            (kv.first->name != kv.first->gateway_name.c_str())) {
                            if (refEntry.first == kv.first->gateway_name.c_str()) {
                                foundTblRef = true;
                                break;
                            }
                        }
                    }

                    if (!foundTblRef) {
                        LOG3("  Not updating physical min/max stages");
                        continue;
                    }
                    if (minPhysicalRead == NOTSET && maxPhysicalRead == NOTSET) {
                        // Initial value not set.
                        minPhysicalRead = physStage;
                        maxPhysicalRead = physStage;
                        continue;
                    }
                    if (physStage < minPhysicalRead) minPhysicalRead = physStage;
                    if (physStage > maxPhysicalRead) maxPhysicalRead = physStage;
                }
            }
            if (usedInDeparser && (stage == deparserMax.first)) {
                if (maxPhysicalRead == NOTSET) maxPhysicalRead = physDeparser;
                if (minPhysicalRead == NOTSET) minPhysicalRead = physDeparser;
            }
        }
        for (auto stage = minStageWritten; stage <= maxStageWritten; stage++) {
            LOG5("\t\t\tWrite Stage: " << stage);
            if (writeTables.count(stage)) {
                for (const auto& kv : writeTables.at(stage)) {
                    // Check that the table is part of the valid stage.
                    if (stage == alloc.getLatestLiveness().first &&
                        alloc.getLatestLiveness().second == read) {
                        LOG3("\t\t  Ignoring written table: " << kv.first->name << ", stage: " <<
                             stage);
                        continue;
                    }
                    bool foundFieldBits = std::any_of(kv.second.begin(), kv.second.end(),
                            [&](le_bitrange range) {
                        return alloc.field_slice().overlaps(range);
                    });
                    if (!foundFieldBits) continue;
                    int physStage = kv.first->stage();
                    LOG3("\t\t  Written table: " << kv.first->name <<
                         ", Phys stage: " << physStage);
                    if (minPhysicalWrite == NOTSET && maxPhysicalWrite == NOTSET) {
                        // Initial value not set, so this stage is both maximum and minimum.
                        minPhysicalWrite = physStage;
                        maxPhysicalWrite = physStage;
                        continue;
                    }

                    if (physStage < minPhysicalWrite) minPhysicalWrite = physStage;
                    if (physStage > maxPhysicalWrite) maxPhysicalWrite = physStage;
                }
            }
        }
        LOG3("\t\tPhys Read: [" << minPhysicalRead << ", " << maxPhysicalRead << "], " <<
             " Phys Write: [" << minPhysicalWrite << ", " << maxPhysicalWrite << "]");
        int new_min_stage, new_max_stage;
        PHV::FieldUse new_min_use, new_max_use;
        bool readAbsent = ((minPhysicalRead == NOTSET) && (maxPhysicalRead == NOTSET));
        bool writeAbsent = ((minPhysicalWrite == NOTSET) && (maxPhysicalWrite == NOTSET));
        if (includeParser && readAbsent && writeAbsent) {
            new_min_stage = 0;
            new_max_stage = 0;
            new_min_use = read;
            new_max_use = read;
            LOG5("\t\t\tParser only setting.");
            PHV::StageAndAccess min = std::make_pair(new_min_stage, new_min_use);
            PHV::StageAndAccess max = std::make_pair(new_max_stage, new_max_use);
            alloc.setLiveness(min, max);
            alloc.setPhysicalDeparserStage(true);
            LOG3("\t  New min stage: " << alloc.getEarliestLiveness().first <<
                 alloc.getEarliestLiveness().second << ", New max stage: " <<
                 alloc.getLatestLiveness().first << alloc.getLatestLiveness().second);
            continue;
        }

        // If there are no unit referencing this slice, use [-1r, 0r]
        // as the trivial liverange which does not impact PHV allocation
        if (readAbsent && writeAbsent && !alwaysRunLastStageSlice) {
            LOG4("\t Slice " << alloc << " does not have any refs and is not last stage ARA");
            new_min_stage = -1;
            new_max_stage = 0;
            new_min_use = read;
            new_max_use = read;
        } else if (readAbsent) {
            // If this slice only has write.
            new_min_stage = minPhysicalWrite;
            new_min_use = write;
            new_max_stage = maxPhysicalWrite;
            new_max_use = write;
        } else if (writeAbsent) {
            // If this slice only has read.
            new_min_stage = minPhysicalRead;
            new_min_use = read;
            new_max_stage = maxPhysicalRead;
            new_max_use = read;
        } else {
            // If this slice both has read and write.
            if (minPhysicalRead <= minPhysicalWrite) {
                new_min_stage = minPhysicalRead;
                new_min_use = read;
            } else {
                new_min_stage = minPhysicalWrite;
                new_min_use = write;
            }
            if (maxPhysicalWrite >= maxPhysicalRead) {
                new_max_stage = maxPhysicalWrite;
                new_max_use = write;
            } else {
                new_max_stage = maxPhysicalRead;
                new_max_use = read;
            }
        }
        if (includeParser) {
            new_min_stage = 0;
            new_min_use = PHV::FieldUse(PHV::FieldUse::READ);
        }
        if (alwaysRunLastStageSlice) {
            new_min_use = write;
            new_max_use = read;
            int dep_stage = depStages.getDeparserStage(f->gress);
            BUG_CHECK(dep_stage >= 0, "No tables detected in the program while finalizing "
                    "PHV allocation");
            LOG5("Found deparser stage " << dep_stage << " for gress " << f->gress);
            new_max_stage = dep_stage;
        }
        PHV::StageAndAccess min = std::make_pair(new_min_stage, new_min_use);
        PHV::StageAndAccess max = std::make_pair(new_max_stage, new_max_use);
        alloc.setLiveness(min, max);
        alloc.setPhysicalDeparserStage(true);
        LOG3("\t  New min stage: " << alloc.getEarliestLiveness().first <<
             alloc.getEarliestLiveness().second << ", New max stage: " <<
             alloc.getLatestLiveness().first << alloc.getLatestLiveness().second);
    }
}


Visitor::profile_t UpdateFieldAllocation::init_apply(const IR::Node* root) {
    fieldToSlicesMap.clear();
    containerToReadStages.clear();
    containerToWriteStages.clear();
    LOG1("Deparser logical stage: " << phv.getDeparserStage());
    parserMin = std::make_pair(-1, PHV::FieldUse(PHV::FieldUse::READ));
    deparserMax = std::make_pair(PhvInfo::getDeparserStage(), PHV::FieldUse(PHV::FieldUse::WRITE));
    if (LOGGING(5)) {
        for (auto& f : phv) {
            LOG5("\tField: " << f.name);
            for (const auto& slice : f.get_alloc())
                LOG5("\t  Slice: " << slice);
        }
    }
    for (auto& f : phv) updateAllocation(&f);
    for (auto& f : phv) {
        LOG1("\tField: " << f.name);
        for (const auto& slice : f.get_alloc())
            LOG1("\t  Slice: " << slice);
    }
    phv.clearMinStageInfo();
    return Inspector::init_apply(root);
}

bool UpdateFieldAllocation::preorder(const IR::MAU::Table* tbl) {
    int stage = tbl->stage();
    PhvInfo::addMinStageEntry(tbl, stage);
    return true;
}

void UpdateFieldAllocation::end_apply() {
    PhvInfo::setDeparserStage(depStages.getDeparserStage());
}

FinalizeStageAllocation::FinalizeStageAllocation(
        PhvInfo& p,
        const FieldDefUse& u,
        const DependencyGraph& d) {
    addPasses({
        &depStages,
        new UpdateFieldAllocation(p, u, d, depStages)
    });
}
