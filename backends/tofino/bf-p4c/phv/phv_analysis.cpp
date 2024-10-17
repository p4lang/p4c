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

#include "phv_analysis.h"

#include <initializer_list>

#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/phv/add_initialization.h"
#include "bf-p4c/phv/cluster_phv_operations.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/parde_phv_constraints.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/table_phv_constraints.h"
#include "bf-p4c/phv/add_special_constraints.h"
#include "bf-p4c/phv/allocate_phv.h"
#include "bf-p4c/phv/validate_allocation.h"
#include "bf-p4c/phv/live_range_split.h"
#include "bf-p4c/phv/analysis/dark.h"
#include "bf-p4c/phv/analysis/deparser_zero.h"
#include "bf-p4c/phv/analysis/memoize_min_stage.h"
#include "bf-p4c/phv/analysis/jbay_phv_analysis.h"
#include "bf-p4c/phv/analysis/mocha.h"
#include "bf-p4c/phv/analysis/mutex_overlay.h"
#include "bf-p4c/phv/v2/phv_allocation_v2.h"
#include "bf-p4c/mau/action_mutex.h"
#include "bf-p4c/logging/event_logger.h"
#include "ir/visitor.h"

class PhvInfo;

/// collect and apply PHV-related global pragmas.
class ApplyGlobalPragmas : public Visitor {
 private:
    PHV::AllocSetting &settings_i;

 protected:
    const IR::Node *apply_visitor(const IR::Node *root_, const char *) override {
        BUG_CHECK(root_->is<IR::BFN::Pipe>(), "IR root is not a BFN::Pipe: %s", root_);
        const auto *root = root_->to<IR::BFN::Pipe>();
        // phv container pragma
        Device::phvSpec().applyGlobalPragmas(root->global_pragmas);
        // single parser gress pragma
        // Check if pragma pa_parser_group_monogress is contained in the p4 program
        for (auto *anno : root->global_pragmas) {
            if (anno->name.name == PragmaParserGroupMonogress::name) {
                settings_i.single_gress_parser_group = true;
            }
            if (anno->name.name == PragmaPrioritizeARAinits::name) {
                settings_i.prioritize_ara_inits = true;
            }
        }
        return root;
    }

 public:
    explicit ApplyGlobalPragmas(PHV::AllocSetting& settings): settings_i(settings){}
};

PHV_AnalysisPass::PHV_AnalysisPass(
        const BFN_Options &options,
        PhvInfo &phv,
        PhvUse &uses,
        const ClotInfo& clot,
        FieldDefUse &defuse,
        DependencyGraph &deps,
        const DeparserCopyOpt &decaf,
        MauBacktracker& alloc,
        CollectPhvLoggingInfo *phvLoggingInfo,
        std::set<PHV::FieldRange> &mauInitFields,
        const TableSummary &table_summary)
    : Logging::PassManager("phv_allocation_"_cs),
      phv_i(phv),
      uses_i(uses),
      clot_i(clot),
      defuse_i(defuse),
      deps_i(deps),
      options_i(options),
      table_alloc(alloc),
      phv_allocation_result(phv, alloc),
      table_replay_phv_constr(alloc, phv, phv_allocation_result.get_trivial_allocation_info(),
        phv_allocation_result.get_real_allocation_info()),
      pragmas(options.alt_phv_alloc
        ? PHV::Pragmas(
            phv,
            // alt-phv-alloc only, table replay fitting may add extra pa_container_size and
            // pa_no_pack pragmas.
            table_replay_phv_constr.get_container_size_constr(),
            table_replay_phv_constr.get_no_pack_constr()
        )
        : PHV::Pragmas(phv)),
      field_to_parser_states(phv),
      parser_critical_path(phv),
      critical_path_clusters(parser_critical_path),
      pack_conflicts(
          phv, deps, table_mutex, alloc, action_mutex, pragmas.pa_no_pack(), &table_summary),
      action_constraints(phv, uses, pack_conflicts, tableActionsMap, deps),
      domTree(flowGraph),
      meta_live_range(phv, deps, defuse, pragmas, uses, alloc),
      non_mocha_dark(phv, uses, defuse, deps.red_info, pragmas),
      dark_live_range(phv, clot, deps, defuse, pragmas, uses, action_constraints, domTree,
                      tableActionsMap, alloc, non_mocha_dark),
      meta_init(phv, defuse, deps, pragmas.pa_no_init(), meta_live_range, action_constraints,
                domTree, alloc),
      clustering(phv, uses, pack_conflicts, pragmas.pa_container_sizes(), pragmas.pa_byte_pack(),
                 action_constraints, defuse, deps, table_mutex, settings, alloc),
      strided_headers(phv),
      tb_keys(phv),
      physical_liverange_db(&alloc, &defuse, phv, deps.red_info, clot, pragmas),
      source_tracker(phv, deps.red_info),
      tablePackOpt(phv),
      utils(phv, clot, clustering, uses, defuse, action_constraints, meta_init, dark_live_range,
            field_to_parser_states, parser_critical_path, parser_info, strided_headers,
            physical_liverange_db, source_tracker, pragmas, settings, tablePackOpt),
      kit(phv, clot, clustering, uses, defuse, action_constraints,
          field_to_parser_states, parser_critical_path, parser_info, strided_headers,
          physical_liverange_db, source_tracker, tb_keys, table_mutex, deps, mauInitFields,
          pragmas, settings, alloc),
      allocate_phv(utils, alloc, phv, unallocated) {
        auto* validate_allocation = new PHV::ValidateAllocation(phv, clot, physical_liverange_db,
            settings);
        addPasses({
            options.alt_phv_alloc ? &table_replay_phv_constr : nullptr,
            // Identify uses of fields in MAU, PARDE
            &uses,
            new PhvInfo::DumpPhvFields(phv, uses),
            // Determine candidates for mocha PHVs.
            Device::phvSpec().hasContainerKind(PHV::Kind::mocha) ? &non_mocha_dark : nullptr,
            Device::phvSpec().hasContainerKind(PHV::Kind::mocha)
                ? new CollectMochaCandidates(phv, uses, deps.red_info, non_mocha_dark) : nullptr,
            Device::phvSpec().hasContainerKind(PHV::Kind::dark)
                ? new CollectDarkCandidates(phv, uses, deps.red_info) : nullptr,
            // Pragmas need to be run here because the later passes may add constraints encoded as
            // pragmas to various fields after the initial pragma processing is done.
            // parse and fold PHV-related pragmas
            &pragmas,
            // Identify fields for deparsed-zero optimization
            new DeparserZeroOptimization(phv, defuse, deps.red_info,
                                         pragmas.pa_deparser_zero(), clot),
            // Produce pairs of mutually exclusive header fields, e.g. (arpSrc, ipSrc)
            new MutexOverlay(phv, pragmas, uses),
            // map fields to parser states
            &field_to_parser_states,
            // calculate ingress/egress parser's critical path
            &parser_critical_path,
            // Refresh dependency graph for live range analysis
            new FindDependencyGraph(phv, deps, &options, ""_cs, "Just Before PHV allocation"_cs),
            new MemoizeStage(deps, alloc),
            // Refresh defuse
            &defuse,
            // Analysis of operations on PHV fields.
            // TODO: Combine with ActionPhvConstraints?
            new PHV_Field_Operations(phv),
            // Mutually exclusive tables information
            &table_mutex,
            // Mutually exclusive action information
            &action_mutex,
            // Collect list of fields that cannot be packed together based on the previous round of
            // table allocation (only useful if we backtracked from table placement to PHV
            // allocation)
            &pack_conflicts,
            &action_constraints,
            &tb_keys,
            // Collect constraints related to the way fields are used in tables.
            new TablePhvConstraints(phv, action_constraints, pack_conflicts),
            // Collect constraints related to the way fields are used in the parser/deparser.
            new PardePhvConstraints(phv, pragmas.pa_container_sizes()),
            &critical_path_clusters,
            // This has to be the last pass in the analysis phase as it adds artificial constraints
            // to fields and uses results of some of the above passes (specifically
            // action_constraints).
            new AddSpecialConstraints(phv, pragmas, action_constraints, decaf),
            // build dominator tree for the program, also populates the flow graph internally.
            &domTree,
            &tableActionsMap,
            // Determine `ideal` live ranges for metadata fields in preparation for live range
            // shrinking that will be effected during and post AllocatePHV.
            &meta_live_range,
            (Device::phvSpec().hasContainerKind(PHV::Kind::dark) &&
                !options.disable_dark_allocation)
                ? &dark_live_range : nullptr,
            // Metadata initialization pass should be run after the metadata live range is
            // calculated.
            options.alt_phv_alloc_meta_init ? (Visitor*)nullptr: (Visitor*)&meta_init,
            // Determine parser constant extract constraints, to be run before Clustering.
            Device::currentDevice() == Device::TOFINO ? new TofinoParserConstantExtract(phv) :
                nullptr,
            new ApplyGlobalPragmas(settings),
            &table_ids,
            &strided_headers,
            &parser_info,
            phvLoggingInfo,
            &physical_liverange_db,
            &source_tracker,
            new PhvInfo::DumpPhvFields(phv, uses),
            &tablePackOpt,
            // From this point on, we are starting to transform the PHV related data structures.
            // Before this is all analysis that collected constraints for PHV allocation to use.
            // &table_friendly_packing_backtracker,                          // <---
            &clustering,                                                     //    |
            options.alt_phv_alloc                                            //    |
                ? (Visitor*)new PHV::v2::PhvAllocation(kit, alloc, phv)      //    |
                : (Visitor*)&allocate_phv,                                   // ----
            options.alt_phv_alloc
                ? nullptr
                : new AddSliceInitialization(phv, defuse, deps, meta_live_range),
            new PHV::LiveRangeSplitOrFail([this]() { return this->allocate_phv.getAllocation(); },
                                          unallocated, phv, utils, deps,
                                          dark_live_range.getLiveMap(), non_mocha_dark),
            &defuse,
            phvLoggingInfo,
            // Validate results of PHV allocation.
            new VisitFunctor([=](){
                validate_allocation->set_physical_liverange_overlay(
                        settings.physical_liverange_overlay);
            }),
            validate_allocation,
            options.alt_phv_alloc ? new PassIf (
            [this]() {
                auto actualState = table_alloc.get_table_summary()->getActualState();
                return actualState == State::ALT_INITIAL
                    || actualState == State::ALT_FINALIZE_TABLE_SAME_ORDER
                    || actualState == State::ALT_FINALIZE_TABLE_SAME_ORDER_TABLE_FIXED;
            },
            {
                // collect trivial phv allocation result.
                &phv_allocation_result
            }) : nullptr,
        });

    phvLoggingInfo->superclusters = &clustering.cluster_groups();
    phvLoggingInfo->pragmas = &pragmas;
    EventLogger::get().iterationChange(invocationCount, EventLogger::AllocPhase::PhvAllocation);
    setName("PHV Analysis");
}

void PHV_AnalysisPass::end_apply() {
    Logging::PassManager::end_apply();
}

namespace {

class IncrementalPHVAllocPass : public Logging::PassManager {
 public:
    IncrementalPHVAllocPass(const std::initializer_list<VisitorRef>& visitors)
        : Logging::PassManager("phv_incremental_allocation_"_cs) {
        addPasses(visitors);
    }
};

}  // namespace

Visitor* PHV_AnalysisPass::make_incremental_alloc_pass(
    const ordered_set<PHV::Field *> &temp_vars) {
    return new IncrementalPHVAllocPass({
         // Warning: phv_i is out of data at this point, but CollectPhvInfo should not be rerun
         // as it will lose uncommitted placement information. (CollectPhvInfo will also change
         // the field objects, causing the fields in temp_vars to no longer point to the intended
         // fields.)
         &tableActionsMap,
         &uses_i,
         // Refresh dependency graph for live range analysis
         new FindDependencyGraph(phv_i, deps_i, &options_i, ""_cs,
                                 "Just Before Incremental PHV allocation"_cs),
         // TODO: MemoizeMinStage will corrupt existing allocslice liverange, because
         // deparser stage is marked as last stage + 1. DO NOT run it.
         &defuse_i,
         &table_mutex,
         &action_mutex,
         &pack_conflicts, &action_constraints,
         new TablePhvConstraints(phv_i, action_constraints, pack_conflicts),
         // &meta_live_range,
         // LiveRangeShrinking pass has its own MapTablesToActions pass, have to rerun it.
         &meta_init,
         // conservative settings to ensure that Redo tableplacement will produce the same result.
         [this]() {
             this->set_trivial_alloc(false);
             this->set_no_code_change(true);
             this->set_physical_liverange_overlay(false);
         },
         new IncrementalPHVAllocation(temp_vars, utils, phv_i, settings)
        });
}
