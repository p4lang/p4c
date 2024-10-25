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

#include "bf-p4c/phv/add_initialization.h"

#include "boost/optional/optional_io.hpp"

bool MapFieldToExpr::preorder(const IR::Expression *expr) {
    if (expr->is<IR::Cast>() || expr->is<IR::Slice>() || expr->is<IR::Neg>()) return true;
    const auto *f = phv.field(expr);
    if (!f) return true;
    fieldExpressions[f->id] = expr;
    return true;
}

const IR::MAU::Instruction *MapFieldToExpr::generateInitInstruction(
    const PHV::AllocSlice &slice) const {
    const auto *f = slice.field();
    BUG_CHECK(f, "Field is nullptr in generateInitInstruction");
    const IR::Expression *zeroExpr = new IR::Constant(IR::Type_Bits::get(slice.width()), 0);
    const IR::Expression *fieldExpr = getExpr(f);
    if (slice.width() == f->size) {
        auto *prim = new IR::MAU::Instruction("set"_cs, {fieldExpr, zeroExpr});
        return prim;
    } else {
        le_bitrange range = slice.field_slice();
        const IR::Expression *sliceExpr = new IR::Slice(fieldExpr, range.hi, range.lo);
        auto *prim = new IR::MAU::Instruction("set"_cs, {sliceExpr, zeroExpr});
        return prim;
    }
}

const IR::MAU::Instruction *MapFieldToExpr::generateInitInstruction(
    const PHV::AllocSlice &dest, const PHV::AllocSlice &source) const {
    const auto *dest_f = dest.field();
    BUG_CHECK(dest_f, "Field is nullptr in generateInitInstruction for dest %1%", dest);
    const auto *source_f = source.field();
    BUG_CHECK(source_f, "Field is nullptr in generateInitInstruction for source %1%", source);
    const IR::Expression *destExpr = getExpr(dest_f);
    const IR::Expression *sourceExpr = getExpr(source_f);
    if (dest.width() != dest_f->size) {
        le_bitrange range = dest.field_slice();
        destExpr = new IR::Slice(destExpr, range.hi, range.lo);
    }
    if (source.width() != source_f->size) {
        le_bitrange range = source.field_slice();
        sourceExpr = new IR::Slice(sourceExpr, range.hi, range.lo);
    }
    auto *prim = new IR::MAU::Instruction("set"_cs, {destExpr, sourceExpr});
    return prim;
}

const IR::Node *ComputeFieldsRequiringInit::apply_visitor(const IR::Node *root, const char *) {
    actionInits.clear();
    fieldsForInit.clear();
    for (auto &f : phv) {
        for (auto &slice : f.get_alloc()) {
            // For each alloc slice in the field, check if metadata initialization is required.
            if (!slice.hasMetaInit()) continue;
            LOG4("\t  Need to initialize " << f.name << " : " << slice);
            fieldsForInit.insert(PHV::FieldSlice(slice.field(), slice.field_slice()));
            for (const auto *act : slice.getInitPoints()) {
                actionInits[act].push_back(slice);
                LOG4("\t\tInitialize at action " << act->name);
            }
        }
    }
    return root;
}

/** This pass determines all the fields to be initialized because of live range shrinking, and
 * adds the relevant initialization operation to the corresponding actions where those fields must
 * be initialized.
 */
class AddMetadataInitialization : public Transform {
 private:
    const MapFieldToExpr &fieldToExpr;
    const ComputeFieldsRequiringInit &fieldsForInit;
    const MapTablesToActions &actionsMap;

    ordered_map<PHV::FieldSlice, PHV::ActionSet> initializedSlices;

    profile_t init_apply(const IR::Node *root) override {
        initializedSlices.clear();
        return Transform::init_apply(root);
    }

    const IR::MAU::Action *postorder(IR::MAU::Action *act) override {
        auto *act_orig = getOriginal<IR::MAU::Action>();
        auto fieldsToBeInited = fieldsForInit.getInitsForAction(act_orig);
        if (fieldsToBeInited.size() == 0) return act;
        // Deduplicate slices here. If they are allocated to the same container (as in the case of
        // deparsed-zero slices), then we only add one initialization to the action.
        ordered_map<PHV::Container, ordered_set<le_bitrange>> allocatedContainerBits;
        std::vector<PHV::AllocSlice> dedupFieldsToBeInitialized;
        for (auto &slice : fieldsToBeInited) {
            if (!allocatedContainerBits.count(slice.container())) {
                allocatedContainerBits[slice.container()].insert(slice.container_slice());
                dedupFieldsToBeInitialized.push_back(slice);
                continue;
            }
            bool addInit = true;
            for (auto bits : allocatedContainerBits.at(slice.container())) {
                if (bits.contains(slice.container_slice())) addInit = false;
            }
            if (!addInit) {
                LOG2("\t\tSlice "
                     << slice
                     << " does not need to be initialized because "
                        "another overlayed slice in the same container is also initialized in "
                        "this action.");
                continue;
            }
            dedupFieldsToBeInitialized.push_back(slice);
        }
        for (auto slice : dedupFieldsToBeInitialized) {
            auto *prim = fieldToExpr.generateInitInstruction(slice);
            if (!prim) {
                warning("Cannot add initialization for slice");
                continue;
            }
            act->action.push_back(prim);
            initializedSlices[PHV::FieldSlice(slice.field(), slice.field_slice())].insert(act);
            if (LOGGING(4)) {
                auto tbl = actionsMap.getTableForAction(act_orig);
                if (!tbl)
                    LOG4("\t\tAdding metadata initialization instruction "
                         << prim << " to action " << act->name << " without table");
                else
                    LOG4("\t\tAdding metadata initialization instruction "
                         << prim << " to action " << act->name << ", in table " << (*tbl)->name);
            }
        }
        return act;
    }

    void end_apply() override {
        if (!LOGGING(2)) return;
        LOG2("\t  Printing all the metadata fields that need initialization with this allocation");
        for (auto &kv : initializedSlices) {
            LOG2("\t\t" << kv.first << " needing initialization at actions:");
            for (const auto *act : kv.second) LOG2("\t\t\t" << act->name);
        }
    }

 public:
    explicit AddMetadataInitialization(const MapFieldToExpr &e, const ComputeFieldsRequiringInit &i,
                                       const MapTablesToActions &a)
        : fieldToExpr(e), fieldsForInit(i), actionsMap(a) {}
};

Visitor::profile_t ComputeDarkInitialization::init_apply(const IR::Node *root) {
    actionToInsertedInsts.clear();
    darkInitToARA.clear();
    phv.clearARAconstraints();
    return Inspector::init_apply(root);
}

void ComputeDarkInitialization::computeInitInstruction(const PHV::AllocSlice &slice,
                                                       const IR::MAU::Action *action) {
    const IR::MAU::Primitive *prim;
    if (slice.getInitPrimitive().destAssignedToZero()) {
        LOG4("\tAdd initialization from zero in action " << action->name << " for: " << slice);
        prim = fieldToExpr.generateInitInstruction(slice);
        LOG4("\t\tAdded initialization: " << prim);
    } else {
        BUG_CHECK(slice.getInitPrimitive().getSourceSlice(),
                  "No source slice defined for allocated slice %1%", slice);
        LOG4("\tAdd initialization in action " << action->name << " for: " << slice);
        LOG4("\t  Initialize from: " << *(slice.getInitPrimitive().getSourceSlice()));
        prim = fieldToExpr.generateInitInstruction(slice,
                                                   *(slice.getInitPrimitive().getSourceSlice()));
        LOG4("\t\tAdded initialization: " << prim);
    }
    auto tbl = tableActionsMap.getTableForAction(action);
    if (!tbl) BUG("No table found corresponding to action %1%", action->name);
    cstring key = getKey(*tbl, action);
    actionToInsertedInsts[key].insert(prim);
}

int ComputeDarkInitialization::calcMinStage(const PHV::AllocSlice &sl_prev,
                                            const PHV::AllocSlice &sl_current, int prior_max_stage,
                                            int post_min_stage, bool init_from_zero) {
    int ara_stg = sl_current.getEarliestLiveness().first;

    if (!init_from_zero) {
        BUG_CHECK(ara_stg == sl_prev.getLatestLiveness().first,
                  "prev slice end stage (%1%) != current slice start stage (%2%)",
                  sl_prev.getLatestLiveness().first, ara_stg);
    }

    BUG_CHECK(prior_max_stage <= ara_stg, "prior_max_stage (%1%) > init stage (%2%)",
              prior_max_stage, ara_stg);
    BUG_CHECK(ara_stg <= post_min_stage, "post_min_stage (%1%) < init stage (%2%)", post_min_stage,
              ara_stg);

    return ara_stg;
}

int ComputeDarkInitialization::ARA_table_id = 0;

bool ComputeDarkInitialization::use_same_containers(PHV::AllocSlice alloc_sl,
                                                    IR::MAU::Table *&ara_tbl) {
    bool share_both_containers = false;
    auto dst_cntr = alloc_sl.container();
    bool has_src = !(!alloc_sl.getInitPrimitive().getSourceSlice());
    auto src_cntr =
        (has_src ? alloc_sl.getInitPrimitive().getSourceSlice()->container() : PHV::Container());
    if (dst_cntr != PHV::Container()) LOG4("\tCurrent AllocSlice dest container: " << dst_cntr);
    if (has_src) LOG4("\tCurrent AllocSlice source container: " << src_cntr);

    for (auto drkInit : darkInitToARA) {
        LOG4("\t Check container sharing with ARA " << drkInit.second->name);

        // Handle dependences between spills/writebacks of different
        // fields to/from the same containers
        if (has_src && (src_cntr == drkInit.first.getDestinationSlice().container()) &&
            alloc_sl.getInitPrimitive().getSourceSlice()->container_slice().overlaps(
                drkInit.first.getDestinationSlice().container_slice())) {
            LOG4("\t\tOverlapping source "
                 << alloc_sl.getInitPrimitive().getSourceSlice() << "\n\t\t to destination "
                 << drkInit.first.getDestinationSlice().container_slice());
        }

        if (drkInit.first.getInitPrimitive().getSourceSlice() &&
            (dst_cntr == drkInit.first.getInitPrimitive().getSourceSlice()->container()) &&
            drkInit.first.getInitPrimitive().getSourceSlice()->container_slice().overlaps(
                alloc_sl.container_slice())) {
            LOG4("\t\tOverlapping destination "
                 << alloc_sl.container_slice() << "\n\t\t to source "
                 << drkInit.first.getInitPrimitive().getSourceSlice()->container_slice());
        }

        // Check destination container
        if (dst_cntr != drkInit.first.getDestinationSlice().container()) continue;
        LOG4("\t\tSame dest container");

        // Check source container
        // Note: The second condition should also match on has_src, but this causes cycles
        // in what should be a DAG.
        if ((drkInit.first.getInitPrimitive().getSourceSlice() &&
             src_cntr != drkInit.first.getInitPrimitive().getSourceSlice()->container()) ||
            (!drkInit.first.getInitPrimitive().getSourceSlice() && has_src))
            continue;
        LOG4("\t\tSame source container");

        ara_tbl = drkInit.second;
        share_both_containers = true;
    }

    return share_both_containers;
}

void ComputeDarkInitialization::createAlwaysRunTable(PHV::AllocSlice &alloc_sl) {
    std::set<UniqueId> prior_tables;
    std::set<UniqueId> post_tables;
    bool use_existing_ara = false;
    bool same_dst_src_cont = false;
    IR::MAU::Table *ara_tbl = nullptr;
    IR::MAU::Action *act = nullptr;
    cstring act_name;
    gress_t ara_gress = alloc_sl.field()->gress;
    auto &init_prim = alloc_sl.getInitPrimitive();
    auto *prev_sl = init_prim.getSourceSlice();
    std::pair<std::set<UniqueId>, std::set<UniqueId>> constraints;

    // 1A. Check if multiple primitives need to be added into the same ARA table
    //     due to combining the spill and the zero-initializations
    //     primitives
    // ---
    LOG4("\t ARA for slice: " << alloc_sl);

    LOG4("\tPrior ARAs:" << init_prim.getARApriorPrims().size());

    for (auto *priorARA : init_prim.getARApriorPrims()) {
        LOG4("\t\t " << *priorARA);

        if (darkInitToARA.count(*priorARA)) {
            LOG4("\t\tFound prior dependent dark prim entry: " << darkInitToARA[*priorARA]->name);

            ara_tbl = darkInitToARA[*priorARA];
            use_existing_ara = true;
        }
    }

    LOG4("\tPost ARAs:" << init_prim.getARApostPrims().size());
    BUG_CHECK(init_prim.getARApostPrims().size() < 2, "More than one post prims?");

    for (auto *postARA : init_prim.getARApostPrims()) {
        LOG4("\t\t " << *postARA);

        if (darkInitToARA.count(*postARA)) {
            LOG4("\t\tFound post dependent dark prim entry: " << darkInitToARA[*postARA]->name);
            BUG_CHECK(!use_existing_ara, "Having both prior and post prims?");

            ara_tbl = darkInitToARA[*postARA];
            use_existing_ara = true;
        }
    }

    // 1B. Create UniqueId sets for ARA table constraints
    // ---
    int prior_max_stage = -1;
    int post_min_stage = PhvInfo::deparser_stage;

    LOG4("\tPrior Tables: ");
    for (auto node : alloc_sl.getInitPrimitive().getARApriorUnits()) {
        const auto *tbl = node->to<IR::MAU::Table>();
        if (!tbl) continue;
        prior_tables.insert(tbl->get_uid());
        LOG4("\t\t" << tbl->name << "  uid:" << tbl->get_uid());

        // Using dependency graph min_stage() instead of PhvInfo::minStage()
        //        due to corner case
        auto minStg = dg.min_stage(tbl);
        prior_max_stage = std::max(prior_max_stage, minStg);
    }

    LOG4("\tPost Tables: ");
    for (auto node : alloc_sl.getInitPrimitive().getARApostUnits()) {
        const auto *tbl = node->to<IR::MAU::Table>();
        if (!tbl) continue;
        post_tables.insert(tbl->get_uid());
        LOG4("\t\t" << tbl->name << "  uid:" << tbl->get_uid());

        for (auto minStg : PhvInfo::minStages(tbl))
            post_min_stage = std::min(post_min_stage, minStg);
    }

    // 1C. Check if multiple primitives need to be added into the same ARA table
    //     due to having the same source and destination containers
    //     Also check if source/dest container is also used by previous or subsequent dark overlays
    //     and update prior/post_tables
    // ---
    IR::MAU::Table *prev_ara_tbl = ara_tbl;
    same_dst_src_cont = use_same_containers(alloc_sl, ara_tbl);

    if (same_dst_src_cont) {
        BUG_CHECK(ara_tbl, "AlwaysRun Table not set ...?");
        int stg = *(PhvInfo::minStages(ara_tbl).begin());

        // Previous ARA stage matches with earliest minStage of this slice
        if (stg == alloc_sl.getEarliestLiveness().first) {
            use_existing_ara = true;
            // Previous ARA stage satisfies min-stage constraints of this slice
        } else if (alloc_sl.getInitPrimitive().getSourceSlice() && (prior_max_stage <= stg) &&
                   (stg <= post_min_stage)) {
            // Need to update the liveranges of the affected slices
            LOG4("\t AllocSlice " << alloc_sl << " may  have its liverange updated ...");

            use_existing_ara = true;
            // Update earliest stage of this slice
            alloc_sl.setEarliestLiveness(
                std::make_pair(stg, alloc_sl.getEarliestLiveness().second));

            if (prev_sl) {
                LOG4("\t Source AllocSlice " << *prev_sl << " may  have its liverange updated ...");
                PHV::Field *p_f = phv.field(prev_sl->field()->id);
                // Find AllocSlice of previously overlaid field
                // and update its liverange according to the new
                // minStage of the ARA
                for (auto &p_sl : p_f->get_alloc()) {
                    if (p_sl.same_alloc_fieldslice(*prev_sl) &&
                        p_sl.getEarliestLiveness().first == prev_sl->getEarliestLiveness().first &&
                        p_sl.getLatestLiveness().first == prev_sl->getLatestLiveness().first) {
                        auto p_stAcc = std::make_pair(stg, p_sl.getLatestLiveness().second);
                        p_sl.setLatestLiveness(p_stAcc);
                        init_prim.setSourceLatestLiveness(p_stAcc);
                        alloc_sl.setInitPrimitive(&init_prim);
                        LOG4("   ... updated source AllocSlice: " << p_sl);
                        LOG4("   ... and primitive in : " << alloc_sl);
                        break;
                    }
                }
            }
        } else if (!init_prim.getSourceSlice()) {
            // Zero-inits shouldn't always be combined since they may occur at different stages
            LOG4("\tRestoring previous ARA table");
            ara_tbl = prev_ara_tbl;
        } else {
            LOG4("\tWARNING: Action Analysis may fail for slice " << alloc_sl);
        }
    }

    // 2. Create or get ARA table
    // ---
    if (use_existing_ara) {
        BUG_CHECK(ara_tbl->actions.size() == 1, "ARA table has more than 1 actions ...?");
        BUG_CHECK(ara_tbl->actions.begin()->second != nullptr, "ARA has null Action ...?");

        act = const_cast<IR::MAU::Action *>(ara_tbl->actions.begin()->second);
        act_name = ara_tbl->actions.begin()->first;

    } else {
        std::stringstream ara_name;
        ara_name << "ara_table_" << ARA_table_id++;
        ara_tbl = new IR::MAU::Table(ara_name, ara_gress);
        ara_tbl->always_run = IR::MAU::AlwaysRun::ACTION;
        ara_name << "_action";
        act_name = cstring(ara_name.str());
        act = new IR::MAU::Action(act_name);
    }

    constraints = std::make_pair(prior_tables, post_tables);

    LOG4("\tAdd ARA initialization in " << (use_existing_ara ? "existing" : "new") << " action "
                                        << act->name << " for: " << alloc_sl);

    const IR::MAU::Instruction *prim;

    if (prev_sl) {
        LOG4("\t  Initialize from: " << *prev_sl);
        prim = fieldToExpr.generateInitInstruction(alloc_sl, *(prev_sl));
    } else {
        LOG4("\t  Initialize from zero ");
        prim = fieldToExpr.generateInitInstruction(alloc_sl);
    }

    LOG4("\t\tAdded initialization: " << prim);
    act->action.push_back(prim);
    ara_tbl->actions[act_name] = act;
    bool no_existing_cnstrs = phv.add_table_constraints(ara_gress, ara_tbl, constraints);

    // Update units of relevant dest and (optionally) source AllocSlices
    LOG5("\t  F. Add unit " << ara_tbl->name.c_str() << " to slice " << alloc_sl);
    alloc_sl.addRef(ara_tbl->name, PHV::FieldUse(PHV::FieldUse::WRITE));
    auto src_sl = alloc_sl.getInitPrimitive().getSourceSlice();
    if (src_sl) {
        auto *fld = phv.field(alloc_sl.field()->id);
        for (auto &fsl : fld->get_alloc()) {
            LOG5("  src_sl = " << *src_sl);
            LOG5("  fsl    = " << fsl);

            if (fsl.same_alloc_fieldslice(*src_sl) &&
                alloc_sl.getEarliestLiveness().first == fsl.getLatestLiveness().first) {
                LOG5("\t  G. Add unit " << ara_tbl->name.c_str() << " to slice " << fsl);
                fsl.addRef(ara_tbl->name, PHV::FieldUse(PHV::FieldUse::READ));
                break;
            }
        }
    }

    // 3. Insert ARA table and constraints into alwaysRunTables
    // ---
    if (!use_existing_ara) {
        int ara_stg = calcMinStage((prev_sl ? *prev_sl : alloc_sl), alloc_sl, prior_max_stage,
                                   post_min_stage, !prev_sl);
        LOG4("\t Prior max stage:" << prior_max_stage << "  Post min stage: " << post_min_stage
                                   << "  ARA stage: " << ara_stg);

        // Update phvInfo::table_to_min_stage (required during ActionAnalysis)
        phv.addMinStageEntry(ara_tbl, ara_stg);

        BUG_CHECK(no_existing_cnstrs, "Table %1% has existing constraints",
                  ara_tbl->externalName());
    }

    // 4. Create mapping from DarkInitEntry to ARA table
    auto *drkinit = new PHV::DarkInitEntry(alloc_sl, alloc_sl.getInitPrimitive());
    darkInitToARA[*drkinit] = ara_tbl;
    LOG4("\t Added darkEntry" << *drkinit << " to " << ara_tbl->name);
}

void ComputeDarkInitialization::end_apply() {
    // *XXX* Use forced_placement to revert back to using regular tables for
    // the spilling part of dark overlays. The main reason is program
    // p4-tests/p4-programs/internal_p4_14/basic_ipv4/basic_ipv4.p4
    // which uses '--placement pragma' and '@pragma stage * 'for most tables
    bool forcedPlacement = BackendOptions().forced_placement;

    for (auto &f : phv) {
        for (auto &slice : f.get_alloc()) {
            // Ignore NOP initializations
            if (slice.getInitPrimitive().isNOP()) continue;

            // Initialization in last MAU stage will be handled directly by another pass.
            // Here we create AlwaysRunAction tables with related placement constraints
            if (slice.getInitPrimitive().mustInitInLastMAUStage()) {
                if (slice.getInitPrimitive().getARApriorUnits().size() ||
                    slice.getInitPrimitive().getARApostUnits().size()) {
                    createAlwaysRunTable(slice);
                }
            } else if (!forcedPlacement && slice.getInitPrimitive().isAlwaysRunActionPrim()) {
                if (slice.getInitPrimitive().getARApriorUnits().size() ||
                    slice.getInitPrimitive().getARApostUnits().size()) {
                    createAlwaysRunTable(slice);
                }
            } else {
                for (const IR::MAU::Action *initAction : slice.getInitPrimitive().getInitPoints())
                    computeInitInstruction(slice, initAction);
            }
        }
    }

    for (auto kv : actionToInsertedInsts) {
        LOG1("\tAction: " << kv.first);
        for (auto *prim : kv.second) LOG1("\t  " << prim);
    }
}

// Retrieve primitives for @tbl/@act hash key
// -----
const ordered_set<const IR::MAU::Primitive *>
ComputeDarkInitialization::getInitializationInstructions(const IR::MAU::Table *tbl,
                                                         const IR::MAU::Action *act) const {
    static ordered_set<const IR::MAU::Primitive *> rv;
    cstring key = getKey(tbl, act);
    if (!actionToInsertedInsts.count(key)) return rv;
    return actionToInsertedInsts.at(key);
}

// Add dark initialization primitives into actions
// -----
class AddDarkInitialization : public Transform {
 private:
    const PhvInfo &phv;
    const ComputeDarkInitialization &initReqs;

    IR::Node *preorder(IR::MAU::Action *action) override {
        const IR::MAU::Table *tbl = findContext<IR::MAU::Table>();
        CHECK_NULL(tbl);
        ordered_set<PHV::FieldSlice> dests;
        for (auto *prim : action->action) {
            le_bitrange range;
            const PHV::Field *f = phv.field(prim->operands[0], &range);
            PHV::FieldSlice slice(f, range);
            dests.insert(slice);
            LOG5("Action " << action->name << " already writes " << slice);
        }
        auto insts = initReqs.getInitializationInstructions(tbl, action);
        for (auto *prim : insts) {
            le_bitrange range;
            const PHV::Field *f = phv.field(prim->operands[0], &range);
            PHV::FieldSlice slice(f, range);
            if (dests.count(slice)) {
                LOG4("Ignoring already added initialization " << slice << " to action "
                                                              << action->name << " and table "
                                                              << (tbl ? tbl->name : "NO TABLE"));
                continue;
            }
            action->action.push_back(prim);
            LOG4("Adding instruction " << prim << " to action " << action->name << " and table "
                                       << (tbl ? tbl->name : "NO TABLE"));
        }
        return action;
    }

 public:
    explicit AddDarkInitialization(const PhvInfo &p, const ComputeDarkInitialization &d)
        : phv(p), initReqs(d) {}
};

void ComputeDependencies::noteDependencies(
    const ordered_map<PHV::AllocSlice, ordered_set<PHV::AllocSlice>> &slices,
    const ordered_map<PHV::AllocSlice, ordered_set<const IR::MAU::Table *>> &initNodes) {
    for (auto &kv : slices) {
        ordered_set<const IR::MAU::Table *> initTables;
        bool noInitFieldFound = false;
        if (!initNodes.count(kv.first)) {
            noInitFieldFound = true;
            LOG5("\tNo init for slice: " << kv.first << ". Have to find uses.");
            for (auto &use : defuse.getAllUses(kv.first.field()->id)) {
                const auto *useTable = use.first->to<IR::MAU::Table>();
                if (!useTable) continue;
                LOG5("\t  Use table for " << kv.first << " : " << useTable->name);
                initTables.insert(useTable);
            }
            for (auto &def : defuse.getAllDefs(kv.first.field()->id)) {
                const auto *defTable = def.first->to<IR::MAU::Table>();
                if (!defTable) continue;
                LOG5("\t  Def table for " << kv.first << " : " << defTable->name);
                initTables.insert(defTable);
            }
        } else {
            initTables.insert(initNodes.at(kv.first).begin(), initNodes.at(kv.first).end());
        }
        for (const auto *initTable : initTables) {
            // initTable = Table where writeSlice is initialized.
            for (const auto &readSlice : kv.second) {
                // For each slice that shares containers with write slice (kv.first)
                for (auto &use : defuse.getAllUses(readSlice.field()->id)) {
                    const auto *readTable = use.first->to<IR::MAU::Table>();
                    if (!readTable) continue;
                    LOG5("\tInit unit for " << kv.first << " : " << initTable->name);
                    LOG5("\t  Read unit for " << readSlice << " : " << readTable->name);
                    if (noInitFieldFound && tableMutex(readTable, initTable)) {
                        LOG5("\t\tIgnoring dependency because of mutually exclusive tables");
                        continue;
                    }
                    if (readTable != initTable) {
                        auto *fieldSlice =
                            new PHV::FieldSlice(readSlice.field(), readSlice.field_slice());
                        phv.addMetadataDependency(readTable, initTable, fieldSlice);
                        LOG5("\t" << readTable->name << " --> " << initTable->name
                                  << " fieldSlice: " << fieldSlice);
                    }
                }
            }
        }
    }
}

Visitor::profile_t ComputeDependencies::init_apply(const IR::Node *root) {
    initTableNames.clear();
    const auto &slices = fieldsForInit.getSlicesRequiringInitialization();
    const auto &livemap = liverange.getMetadataLiveMap();
    // Set of fields involved in metadata initialization whose usedef live ranges must be
    // considered.
    ordered_map<PHV::AllocSlice, ordered_set<PHV::AllocSlice>> initSlicesToOverlappingSlices;

    if (!phv.alloc_done())
        BUG("ComputeDependencies pass must be called after PHV allocation is complete.");

    // Generate the initSlicesToOverlappingSlices map.
    // The key here is a field that must be initialized as part of live range shrinking; the values
    // are the set of fields that overlap with the key field, and so there must be dependencies
    // inserted from the reads of the value fields to the initializations inserted.
    for (const auto &sliceToInit : slices) {
        const PHV::Field *f = sliceToInit.field();
        // For each field, check the other slices overlapping with that field.
        // FIXME(cc): check that this should always be the full pipeline. Or this should change
        // when we compute live ranges per stage
        f->foreach_alloc([&](const PHV::AllocSlice &slice) {
            if (!slice.field_slice().overlaps(sliceToInit.range())) return;
            auto slices = phv.get_slices_in_container(slice.container());
            for (auto &sl : slices) {
                if (slice == sl) continue;
                // If parser mutual exclusion, then we do not need to consider these fields for
                // metadata initialization.
                if (phv.isFieldMutex(f, sl.field())) continue;
                // If slices do not overlap, then ignore.
                if (!slice.container_slice().overlaps(sl.container_slice())) continue;
                // Check live range here.
                auto liverange1 = livemap.at(f->id);
                auto liverange2 = livemap.at(sl.field()->id);
                // If the fields to be initialized has an earlier
                // liverange --> no initization is needed
                if (liverange1.first <= liverange2.first && liverange1.second <= liverange2.first) {
                    LOG3("\t  Ignoring field "
                         << sl.field()->name << " (" << liverange2.first << ", "
                         << liverange2.second << ")  with earlier liverange than " << f->name
                         << " (" << liverange1.first << ", " << liverange1.second
                         << ") due to live ranges");
                    continue;
                }
                if (!(liverange1.first >= liverange2.second &&
                      liverange1.second >= liverange2.second)) {
                    LOG3("\t  Ignoring field " << sl.field()->name << " (" << liverange2.first
                                               << ", " << liverange2.second << ") overlapping with "
                                               << f->name << " (" << liverange1.first << ", "
                                               << liverange1.second << ") due to live ranges");
                    continue;
                }
                PHV::AllocSlice initSlice(phv.field(slice.field()->id), slice.container(),
                                          slice.field_slice().lo, slice.container_slice().lo,
                                          slice.width());
                PHV::AllocSlice overlappingSlice(phv.field(sl.field()->id), sl.container(),
                                                 sl.field_slice().lo, sl.container_slice().lo,
                                                 sl.width());
                initSlicesToOverlappingSlices[initSlice].insert(overlappingSlice);
            }
        });
    }

    for (auto &kv : initSlicesToOverlappingSlices) {
        LOG5("  Initialize slice " << kv.first);
        for (auto &sl : kv.second) LOG5("\t" << sl);
    }

    // Generate init slice to table map.
    ordered_map<PHV::AllocSlice, ordered_set<const IR::MAU::Table *>> slicesToTableInits;
    const auto &initMap = fieldsForInit.getAllActionInits();
    for (auto kv : initMap) {
        auto t = actionsMap.getTableForAction(kv.first);
        if (!t) BUG("Cannot find table corresponding to action %1%", kv.first->name);
        for (const auto &slice : kv.second) {
            PHV::AllocSlice initSlice(phv.field(slice.field()->name), slice.container(),
                                      slice.field_slice().lo, slice.container_slice().lo,
                                      slice.width());
            slicesToTableInits[initSlice].insert(*t);
        }
    }

    for (auto &kv : slicesToTableInits) {
        LOG5("  Initializing slice " << kv.first);
        for (const auto *t : kv.second) LOG5("\t" << t->name);
    }

    noteDependencies(initSlicesToOverlappingSlices, slicesToTableInits);

    if (Device::phvSpec().hasContainerKind(PHV::Kind::dark)) addDepsForDarkInitialization();

    return Inspector::init_apply(root);
}

void ComputeDependencies::summarizeDarkInits(StageFieldUse &fieldWrites,
                                             StageFieldUse &fieldReads) {
    // Add all the initializations to be inserted for dark primitives to the fieldWrites and
    // fieldReads maps.
    for (auto &f : phv) {
        f.foreach_alloc([&](const PHV::AllocSlice &alloc) {
            if (alloc.getInitPrimitive().getInitPoints().size() == 0) return;
            if (alloc.getInitPrimitive().isNOP()) return;
            // if (alloc.getInitPrimitive().mustInitInLastMAUStage()) return;
            ordered_set<const IR::MAU::Table *> initTables;
            LOG5("DarkInits for slice " << alloc << " in tables:");
            for (const auto *action : alloc.getInitPrimitive().getInitPoints()) {
                auto t = actionsMap.getTableForAction(action);
                BUG_CHECK(t, "No table corresponding to action %1%", action->name);
                initTables.insert(*t);
                initTableNames.insert((*t)->name);
                LOG5("\t" << (*t)->name);
            }
            if (alloc.getInitPrimitive().destAssignedToZero()) {
                for (const auto *t : initTables) {
                    fieldWrites[&f][dg.min_stage(t)][t].insert(alloc.field_slice());
                    LOG5("\t\t inserting zero write for table " << t->name);
                }
            }
            if (alloc.getInitPrimitive().getSourceSlice()) {
                const PHV::Field *src = alloc.getInitPrimitive().getSourceSlice()->field();
                for (const auto *t : initTables) {
                    fieldReads[src][dg.min_stage(t)][t].insert(alloc.field_slice());
                    LOG5("\t\t inserting read for table " << t->name << " of " << *src);
                }
            }
        });
    }
}

void ComputeDependencies::addDepsForDarkInitialization() {
    static PHV::FieldUse READ(PHV::FieldUse::READ);
    static PHV::FieldUse WRITE(PHV::FieldUse::WRITE);
    static std::pair<int, PHV::FieldUse> parserMin = std::make_pair(-1, READ);
    // Don't make this static: the deparser stage can be different for each pipe
    // (and maybe also for different passes?)
    std::pair<int, PHV::FieldUse> deparserMax = std::make_pair(PhvInfo::getDeparserStage(), WRITE);
    // Summarize use defs for all fields.
    StageFieldUse fieldWrites;
    StageFieldUse fieldReads;
    ordered_map<PHV::Container, std::vector<PHV::AllocSlice>> containerToSlicesMap;
    ordered_map<const PHV::Field *, std::vector<PHV::AllocSlice>> fieldToSlicesMap;
    for (auto &f : phv) {
        LOG5("\t" << f.name);
        fieldWrites[&f] = StageFieldEntry();
        fieldReads[&f] = StageFieldEntry();
        bool usedInParser = false, usedInDeparser = false;
        FinalizeStageAllocation::summarizeUseDefs(phv, dg, defuse.getAllDefs(f.id), fieldWrites[&f],
                                                  usedInParser, usedInDeparser,
                                                  false /* usePhysicalStages */);
        FinalizeStageAllocation::summarizeUseDefs(phv, dg, defuse.getAllUses(f.id), fieldReads[&f],
                                                  usedInParser, usedInDeparser,
                                                  false /* usePhysicalStages */);
        if (fieldWrites.at(&f).size() > 0) LOG5("\t  Write tables:");
        for (auto &kv : fieldWrites.at(&f))
            for (auto &kv1 : kv.second) LOG5("\t\t" << kv.first << " : " << kv1.first->name);
        if (fieldReads.at(&f).size() > 0) LOG5("\t Read tables:");
        for (auto &kv : fieldReads.at(&f))
            for (auto &kv1 : kv.second) LOG5("\t\t" << kv.first << " : " << kv1.first->name);
        f.foreach_alloc([&](const PHV::AllocSlice &alloc) {
            if (parserMin == alloc.getEarliestLiveness() &&
                deparserMax == alloc.getLatestLiveness())
                return;
            containerToSlicesMap[alloc.container()].push_back(alloc);
            fieldToSlicesMap[&f].push_back(alloc);
        });
    }
    summarizeDarkInits(fieldWrites, fieldReads);
    // Sort slices in each container according to liveness.
    for (auto &kv : containerToSlicesMap) {
        if (kv.second.size() == 1) continue;
        LOG5("\tContainer: " << kv.first);
        std::sort(kv.second.begin(), kv.second.end(),
                  [](const PHV::AllocSlice &lhs, const PHV::AllocSlice &rhs) {
                      if (lhs.getEarliestLiveness().first != rhs.getEarliestLiveness().first)
                          return lhs.getEarliestLiveness().first < rhs.getEarliestLiveness().first;
                      return lhs.getEarliestLiveness().second < rhs.getEarliestLiveness().second;
                  });
        addDepsForSetsOfAllocSlices(kv.second, fieldWrites, fieldReads);
    }
    for (auto &kv : fieldToSlicesMap) {
        if (kv.second.size() == 1) continue;
        LOG5("\tField: " << kv.first);
        std::sort(kv.second.begin(), kv.second.end(),
                  [](const PHV::AllocSlice &lhs, const PHV::AllocSlice &rhs) {
                      if (lhs.getEarliestLiveness().first != rhs.getEarliestLiveness().first)
                          return lhs.getEarliestLiveness().first < rhs.getEarliestLiveness().first;
                      return lhs.getEarliestLiveness().second < rhs.getEarliestLiveness().second;
                  });
        addDepsForSetsOfAllocSlices(kv.second, fieldWrites, fieldReads, false);
    }
}

void ComputeDependencies::accountUses(int min_stage, int max_stage, const PHV::AllocSlice &alloc,
                                      const StageFieldUse &uses,
                                      ordered_set<const IR::MAU::Table *> &tables) const {
    for (int stage = min_stage; stage <= max_stage; ++stage) {
        if (!uses.at(alloc.field()).count(stage)) continue;
        for (const auto &kv : uses.at(alloc.field()).at(stage)) {
            bool foundFieldBits =
                std::any_of(kv.second.begin(), kv.second.end(),
                            [&](le_bitrange range) { return alloc.field_slice().overlaps(range); });
            if (foundFieldBits) tables.insert(kv.first);
        }
    }
}

void ComputeDependencies::addDepsForSetsOfAllocSlices(
    const std::vector<PHV::AllocSlice> &alloc_slices, const StageFieldUse &fieldWrites,
    const StageFieldUse &fieldReads, bool checkBitsOverlap) {
    static PHV::FieldUse READ(PHV::FieldUse::READ);
    static PHV::FieldUse WRITE(PHV::FieldUse::WRITE);
    // TODO: Handle the case where:
    // C1[0] <-- f1 {1, 2}
    // C1[1] <-- f2 {1, 3}
    // C1 <-- f3 {3, 7}
    if (LOGGING(5)) {
        LOG5("\t\t Slices:");
        for (auto slc : alloc_slices) LOG5("\t\t\t" << slc);
    }

    for (unsigned i = 0; i < alloc_slices.size() - 1; ++i) {
        const PHV::AllocSlice &alloc = alloc_slices.at(i);
        if (!fieldWrites.count(alloc.field()) && !fieldReads.count(alloc.field())) continue;
        ordered_set<const IR::MAU::Table *> allocUses;
        ordered_set<PHV::AllocSlice> next_dep_slices;

        // Gather all the usedefs for alloc.
        if (fieldReads.count(alloc.field())) {
            int minStageRead = (alloc.getEarliestLiveness().second == READ)
                                   ? alloc.getEarliestLiveness().first
                                   : (alloc.getEarliestLiveness().first + 1);
            accountUses(minStageRead, alloc.getLatestLiveness().first, alloc, fieldReads,
                        allocUses);
        }
        if (fieldWrites.count(alloc.field())) {
            int maxStageWritten =
                (alloc.getLatestLiveness().second == WRITE)
                    ? alloc.getLatestLiveness().first
                    : (alloc.getLatestLiveness().first == 0 ? 0
                                                            : alloc.getLatestLiveness().first - 1);
            accountUses(alloc.getEarliestLiveness().first, maxStageWritten, alloc, fieldWrites,
                        allocUses);
        }
        if (allocUses.size() > 0) {
            LOG5("\t\tInsert dependencies from following usedefs of " << alloc);
            for (const auto *t : allocUses)
                LOG5("\t\t\t" << t->name << " (Stage " << dg.min_stage(t) << ")");
        }

        // Find the initialization point for the next alloc slice.
        for (unsigned j = i + 1; j < alloc_slices.size(); ++j) {
            const PHV::AllocSlice &nextAlloc = alloc_slices.at(j);
            ordered_set<const IR::MAU::Table *> nextAllocUses;

            if (checkBitsOverlap &&
                !nextAlloc.container_slice().overlaps(alloc.container_slice())) {
                LOG5("\t\tIgnoring alloc " << nextAlloc
                                           << " because the previous alloc and this "
                                              "one do not overlap in the container.");
                continue;
            }
            if (!checkBitsOverlap && !nextAlloc.field_slice().overlaps(alloc.field_slice())) {
                LOG5("\t\tIgnoring alloc " << nextAlloc
                                           << " because the previous alloc belongs"
                                              " to a different slice of the field.");
                continue;
            }
            if (nextAlloc.getInitPrimitive().isNOP()) {
                // Add all the uses of the field to the set of fields to which the dependencies
                // must be inserted.
                LOG5("\t\tAdd dependencies to all usedefs of " << nextAlloc);
                // Gather all usedefs for nextAlloc.
                if (fieldReads.count(nextAlloc.field())) {
                    int minStageRead = (nextAlloc.getEarliestLiveness().second == READ)
                                           ? nextAlloc.getEarliestLiveness().first
                                           : (nextAlloc.getEarliestLiveness().first + 1);
                    accountUses(minStageRead, nextAlloc.getLatestLiveness().first, nextAlloc,
                                fieldReads, nextAllocUses);
                }
                if (fieldWrites.count(nextAlloc.field())) {
                    int maxStageWritten = (nextAlloc.getLatestLiveness().second == WRITE)
                                              ? nextAlloc.getLatestLiveness().first
                                              : (nextAlloc.getLatestLiveness().first == 0
                                                     ? 0
                                                     : nextAlloc.getLatestLiveness().first - 1);
                    accountUses(nextAlloc.getEarliestLiveness().first, maxStageWritten, nextAlloc,
                                fieldWrites, nextAllocUses);
                }
                for (const auto *t : nextAllocUses)
                    LOG5("\t\t\t" << t->name << " (Stage " << dg.min_stage(t) << ")");
            } else if (nextAlloc.getInitPrimitive().mustInitInLastMAUStage()) {
                // No need for any dependencies here.
                LOG5("\t\tNo need to insert dependencies to the last always_init block.");
            } else if (nextAlloc.getInitPrimitive().getInitPoints().size() > 0) {
                const IR::MAU::Table *initTable = nullptr;
                for (const auto *action : nextAlloc.getInitPrimitive().getInitPoints()) {
                    auto tbl = actionsMap.getTableForAction(action);
                    BUG_CHECK(tbl, "No table corresponding to action %1%", action->name);
                    if (initTable)
                        BUG_CHECK(initTable == *tbl, "Multiple tables found for dark primitive");
                    initTable = *tbl;
                }
                LOG5("\t\tAdd dependencies to initialization point "
                     << initTable->name << " (Stage " << dg.min_stage(initTable) << ")");
                nextAllocUses.insert(initTable);
            }

            // Add dependencies between tables of AllocSlice and next sorted AllocSlice
            for (const auto *fromDep : allocUses) {
                for (const auto *toDep : nextAllocUses) {
                    if (fromDep == toDep) continue;

                    // Check if an earlier dependent slice has dependencies to nextAlloc also ...
                    bool intermediate_dep = false;
                    for (auto dep_slc : next_dep_slices) {
                        if (nextAlloc.container_slice().overlaps(dep_slc.container_slice())) {
                            intermediate_dep = true;
                            break;
                        }
                    }
                    // ... amd if it does avoid adding dependencies.
                    // That is only add dependencies between temporaly neighboring slices.
                    // I.e. for 3 slices in liverange sorted order s1, s2, s3 with dependences
                    // between each pair of slices, only add dependences s1-->s2, s2-->s3 and
                    // skip dependence s1-->s3
                    if (intermediate_dep) continue;

                    LOG4("\t\tAdd dep from " << fromDep->name << " --> " << toDep->name);
                    phv.addMetadataDependency(fromDep, toDep);
                }
            }
            // Keep track of Slices we add container confict dependences to
            next_dep_slices.insert(nextAlloc);
        }
    }
}

void ComputeDependencies::end_apply() {
    LOG1("\t  Printing new dependencies to be inserted");
    for (auto kv : phv.getMetadataDeps())
        for (cstring t : kv.second) LOG1("\t\t" << kv.first << " --> " << t);

    LOG3("\t  Printing reverse metadata deps");
    for (auto kv : phv.getReverseMetadataDeps()) {
        std::stringstream ss;
        ss << "\t\t" << kv.first << " : ";
        for (auto t : kv.second) ss << t << " ";
        LOG3(ss.str());
    }
    LOG7(PhvInfo::reportMinStages());
}

AddSliceInitialization::AddSliceInitialization(PhvInfo &p, FieldDefUse &d, const DependencyGraph &g,
                                               const MetadataLiveRange &r)
    : fieldToExpr(p),
      init(p),
      dep(p, g, actionsMap, init, d, r, tableMutex),
      computeDarkInit(p, actionsMap, fieldToExpr, g) {
    addPasses({&tableMutex, &actionsMap, &fieldToExpr, &init,
               new AddMetadataInitialization(fieldToExpr, init, actionsMap),
               Device::phvSpec().hasContainerKind(PHV::Kind::dark) ? &computeDarkInit : nullptr,
               Device::phvSpec().hasContainerKind(PHV::Kind::dark)
                   ? new AddDarkInitialization(p, computeDarkInit)
                   : nullptr,
               Device::phvSpec().hasContainerKind(PHV::Kind::dark)
                   ? new AddAlwaysRun(p.getARAConstraints())
                   : nullptr,
               &dep, new MarkDarkInitTables(dep)});
}
