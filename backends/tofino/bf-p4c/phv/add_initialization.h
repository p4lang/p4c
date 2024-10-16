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

#ifndef EXTENSIONS_BF_P4C_PHV_ADD_INITIALIZATION_H_
#define EXTENSIONS_BF_P4C_PHV_ADD_INITIALIZATION_H_

#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/map_tables_to_actions.h"
#include "bf-p4c/phv/finalize_stage_allocation.h"
#include "bf-p4c/phv/analysis/meta_live_range.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/mau/add_always_run.h"

/** A helper class that maps PHV::Field to an IR::Expression.
  * It also provides a helper function to generate an IR::MAU::Instruction* to initialize a field to
  * 0.
  */
class MapFieldToExpr : public Inspector {
 private:
    const PhvInfo& phv;
    ordered_map<int, const IR::Expression*> fieldExpressions;

    profile_t init_apply(const IR::Node* root) override {
        fieldExpressions.clear();
        return Inspector::init_apply(root);
    }

    /// For every IR::Expression object in the program, populate the fieldExpressions map. This
    /// might return a Field for slice and cast expressions on fields. It will work out okay solely
    /// because this is a preorder and we don't prune, so it will also visit the child, which is the
    /// field, and then replace the entry in the map.
    bool preorder(const IR::Expression* expr) override;

 public:
    explicit MapFieldToExpr(const PhvInfo& p) : phv(p) { }

    const IR::Expression* getExpr(const PHV::Field* field) const {
        BUG_CHECK(fieldExpressions.count(field->id),
                  "Missing IR::Expression mapping of %1%", field->name);
        return fieldExpressions.at(field->id)->clone();
    }

    /// @returns an instruction that initializes the field slice @p slice.
    const IR::MAU::Instruction*
        generateInitInstruction(const PHV::AllocSlice& slice) const;

    const IR::MAU::Instruction* generateInitInstruction(
            const PHV::AllocSlice& dest,
            const PHV::AllocSlice& source) const;
};

class ComputeFieldsRequiringInit : public Visitor {
 private:
    const PhvInfo&                  phv;

    // Map of action names to the set of fields that must be initialized at that action.
    ordered_map<const IR::MAU::Action*, std::vector<PHV::AllocSlice>> actionInits;

    // Map of all fields that must be initialized (across all actions).
    ordered_set<PHV::FieldSlice> fieldsForInit;

    const IR::Node *apply_visitor(const IR::Node* root, const char * = nullptr) override;

 public:
    explicit ComputeFieldsRequiringInit(const PhvInfo& p) : phv(p) { }

    // @returns the set of all fields that must be initialized across all actions.
    const ordered_set<PHV::FieldSlice>& getSlicesRequiringInitialization() const {
        return fieldsForInit;
    }

    // @returns the map of all actions and the metadata fields to be initialized in those actions.
    const ordered_map<const IR::MAU::Action*, std::vector<PHV::AllocSlice>>&
    getAllActionInits() const {
        return actionInits;
    }

    // @returns the set of fields that must be initialized at action @act.
    const std::vector<PHV::AllocSlice>
    getInitsForAction(const IR::MAU::Action* act) const {
        std::vector<PHV::AllocSlice> empty;
        if (!actionInits.count(act))
            return empty;
        return actionInits.at(act);
    }
};

class ComputeDarkInitialization : public Inspector {
 private:
    PhvInfo&                    phv;
    const MapTablesToActions&   tableActionsMap;
    const MapFieldToExpr&       fieldToExpr;
    const DependencyGraph&      dg;

    ordered_map<cstring, ordered_set<const IR::MAU::Primitive*>> actionToInsertedInsts;
    ordered_map<const PHV::DarkInitEntry, IR::MAU::Table*> darkInitToARA;

    void computeInitInstruction(
        const PHV::AllocSlice& slice,
        const IR::MAU::Action* act);

    // Calculate the minStage for the ARA table moving a slice between
    // normal/dark PHVs
    //  Ths will need to be updated with more detailed analysis when
    //        the initialization injection stops using the dominator analysis
    int calcMinStage(const PHV::AllocSlice &sl_prev, const PHV::AllocSlice &sl_current,
                     int prior_max_stage, int post_min_stage, bool init_from_zero);

    // Check dest and src containers if they match with containers in
    // darkInitToARA AllocSlice's
    bool use_same_containers(PHV::AllocSlice alloc_sl, IR::MAU::Table *&ara_tbl);

    // Create new MOVE instruction and place it into new Action which
    // is added into new AlwaysRunAction Table. Use prior and post
    // Tables from @alloc_sl to set the minStage for the new AlwaysRunAction Table
    void createAlwaysRunTable(PHV::AllocSlice& alloc_sl);

    cstring getKey(const IR::MAU::Table* tbl, const IR::MAU::Action* act) const {
        return (tbl->name + "." + act->name);
    }

    profile_t init_apply(const IR::Node* root) override;
    void end_apply() override;

 public:
    // Increasing integer value used in AlwaysRunAction Table name
    static int ARA_table_id;
    explicit ComputeDarkInitialization(
            PhvInfo& p,
            const MapTablesToActions& m,
            const MapFieldToExpr& e,
            const DependencyGraph& g
        )
        : phv(p), tableActionsMap(m), fieldToExpr(e), dg(g) { }

    const ordered_set<const IR::MAU::Primitive*>
    getInitializationInstructions(const IR::MAU::Table* tbl, const IR::MAU::Action* act) const;
};

/** LiveRangeShrinking makes assumptions about the disjoint live ranges of metadata fields
  * (separated by the compiler-defined dependence distance). For each pair of fields overlapping in
  * a container due to live range shrinking, we need to maintain the invariant that the uses of the
  * field with the earlier live range must all happen before the initializations of the field with
  * the later live range (initialization by definition is the first def of the field, and the field
  * ceases to have uninitialized reads after metadata initialization). This is equivalent to
  * creating new dependencies between the earlier field use tables and the tables initializing the
  * next field.
  *
  * This dependency information is currently memoized in the PhvInfo object by this pass. In the
  * long run, this could be (and probably should be) added to the DependencyGraph structure.
  */
class ComputeDependencies : public Inspector {
 private:
    using TableExprRanges = FinalizeStageAllocation::TableExprRanges;
    using StageFieldEntry = FinalizeStageAllocation::StageFieldEntry;
    using StageFieldUse = FinalizeStageAllocation::StageFieldUse;

    PhvInfo&                                    phv;
    const DependencyGraph&                      dg;
    const MapTablesToActions&                   actionsMap;
    const ComputeFieldsRequiringInit&           fieldsForInit;
    const FieldDefUse&                          defuse;
    const MetadataLiveRange&                    liverange;
    const TablesMutuallyExclusive&              tableMutex;

    ordered_set<cstring>                        initTableNames;

    profile_t init_apply(const IR::Node* root) override;
    void end_apply() override;

    // @fields: Map of a field being initialized to the overlapping fields.
    // @inits: Map of a field being initialized to the tables where the initialization is inserted.
    // Note down all the dependencies that would be induced because of the initialization.
    void noteDependencies(
            const ordered_map<PHV::AllocSlice, ordered_set<PHV::AllocSlice>>& slices,
            const ordered_map<PHV::AllocSlice, ordered_set<const IR::MAU::Table*>>& initNodes);

    void addDepsForDarkInitialization();
    void addDepsForSetsOfAllocSlices(
            const std::vector<PHV::AllocSlice>& alloc_slices,
            const StageFieldUse& fieldWrites,
            const StageFieldUse& fieldReads,
            bool checkBitsOverlap = true);
    // Given a @min_stage, a @max_stage, and a map @uses of the uses/defs of slice @alloc,
    // populate @tables with all the tables using that slice between the min and max stage
    // range.
    void accountUses(int min_stage, int max_stage,
            const PHV::AllocSlice& alloc,
            const StageFieldUse& uses,
            ordered_set<const IR::MAU::Table*>& tables) const;
    void summarizeDarkInits(StageFieldUse& fieldWrites, StageFieldUse& fieldReads);

 public:
    ComputeDependencies(
            PhvInfo& p,
            const DependencyGraph& g,
            const MapTablesToActions& a,
            const ComputeFieldsRequiringInit& i,
            const FieldDefUse& d,
            const MetadataLiveRange& r,
            const TablesMutuallyExclusive& m)
        : phv(p), dg(g), actionsMap(a), fieldsForInit(i), defuse(d), liverange(r), tableMutex(m) { }

    bool isInitTable(const IR::MAU::Table* tbl) const {
        return initTableNames.count(tbl->name);
    }
};

/** Mark all the tables which have actions containing initializations due to dark overlay. These
  * tables cannot be split into multiple stages during TablePlacement.
  * TODO: Remove after implementing always_run mechanism on non-last stages.
  */
class MarkDarkInitTables : public Transform {
    const ComputeDependencies&  dep;

    IR::Node* preorder(IR::MAU::Table* tbl) override {
        if (dep.isInitTable(tbl)) {
            tbl->has_dark_init = true;
            LOG2("\tDark initialization table: " << tbl->name);
        }
        return tbl;
    }

 public:
    explicit MarkDarkInitTables(const ComputeDependencies& d) : dep(d) { }
};

/** This is the pass manager, which takes the results of PHV allocation and then inserts the right
  * initialization for metadata fields into various initialization points (actions). This must be
  * run after the AllocatePHV pass.
  */
class AddSliceInitialization : public PassManager {
 private:
    TablesMutuallyExclusive     tableMutex;
    MapTablesToActions          actionsMap;
    MapFieldToExpr              fieldToExpr;
    ComputeFieldsRequiringInit  init;
    ComputeDependencies         dep;
    ComputeDarkInitialization   computeDarkInit;

 public:
    explicit AddSliceInitialization(
            PhvInfo& p,
            FieldDefUse& d,
            const DependencyGraph& g,
            const MetadataLiveRange& r);
};

#endif  /* EXTENSIONS_BF_P4C_PHV_ADD_INITIALIZATION_H_ */
