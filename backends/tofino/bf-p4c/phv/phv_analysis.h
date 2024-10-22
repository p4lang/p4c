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

/**
 * \defgroup phv_allocatioin PHV allocation
 * \brief Content related to %PHV allocation
 *
 * TODO High-level description of %PHV allocation
 */

#ifndef BF_P4C_PHV_PHV_ANALYSIS_H_
#define BF_P4C_PHV_PHV_ANALYSIS_H_

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/map_tables_to_actions.h"
#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/logging/phv_logging.h"
#include "bf-p4c/mau/action_mutex.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_flow_graph.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/decaf.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/action_source_tracker.h"
#include "bf-p4c/phv/allocate_phv.h"
#include "bf-p4c/phv/analysis/critical_path_clusters.h"
#include "bf-p4c/phv/analysis/dark_live_range.h"
#include "bf-p4c/phv/analysis/dominator_tree.h"
#include "bf-p4c/phv/analysis/live_range_shrinking.h"
#include "bf-p4c/phv/analysis/meta_live_range.h"
#include "bf-p4c/phv/analysis/non_mocha_dark_fields.h"
#include "bf-p4c/phv/analysis/pack_conflicts.h"
#include "bf-p4c/phv/analysis/parser_critical_path.h"
#include "bf-p4c/phv/collect_strided_headers.h"
#include "bf-p4c/phv/collect_table_keys.h"
#include "bf-p4c/phv/fieldslice_live_range.h"
#include "bf-p4c/phv/make_clusters.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "bf-p4c/phv/utils/tables_to_ids.h"
#include "bf-p4c/phv/v2/phv_kit.h"
#include "bf-p4c/phv/v2/table_replay_friendly_constraints.h"

/** This is the main PHV allocation pass manager.
 */
class PHV_AnalysisPass : public Logging::PassManager {
 private:
    PhvInfo &phv_i;
    PhvUse &uses_i;
    const ClotInfo &clot_i;
    FieldDefUse &defuse_i;
    DependencyGraph &deps_i;
    const BFN_Options &options_i;
    /// Contains information about placement of tables by an earlier table allocation pass.
    MauBacktracker &table_alloc;
    // alt-phv-alloc ONLY
    // collect information about which field is allocated to what containers at the end of
    // phv analysis
    PHV::v2::CollectPHVAllocationResult phv_allocation_result;
    // alt-phv-alloc ONLY
    // Based on the feedback from table summary after table replay failed, it adds phv container
    // size constraints so that table replay can progress.
    PHV::v2::TableReplayFriendlyPhvConstraints table_replay_phv_constr;
    /// PHV related pragma information.
    PHV::Pragmas pragmas;
    /// Fields to parser states in which they are extracted
    MapFieldToParserStates field_to_parser_states;
    /// Parser critical path of both ingress/egress.
    CalcParserCriticalPath parser_critical_path;
    /// Critical clusters.
    CalcCriticalPathClusters critical_path_clusters;
    /// Mutual exclusion information for tables.
    TablesMutuallyExclusive table_mutex;
    /// Mutual exclusion information for actions.
    ActionMutuallyExclusive action_mutex;
    /// No pack conflicts generated from table allocation pass.
    PackConflicts pack_conflicts;
    /// Action induced packing constraints.
    ActionPhvConstraints action_constraints;
    /// Table flow graph for each gress in the program.
    ordered_map<gress_t, FlowGraph> flowGraph;
    /// Dominator tree for the program.
    BuildDominatorTree domTree;
    /// Map of tables to actions and vice versa.
    MapTablesToActions tableActionsMap;
    /// Metadata live range overlay potential information based on table dependency graph.
    MetadataLiveRange meta_live_range;
    // Gets fields that are not mocha and/or dark compatible.
    NonMochaDarkFields non_mocha_dark;
    /// Identification of fields that could be allocated to the same container pending the copying
    /// of one or more fields into a dark container.
    DarkOverlay dark_live_range;
    /// Metadata initialization related pass.
    LiveRangeShrinking meta_init;
    /// Clustering information for fields.
    Clustering clustering;
    /// Tables To IDs used in PHV analysis.
    MapTablesToIDs table_ids;
    /// Collect header stacks that need strided allocation
    CollectStridedHeaders strided_headers;
    CollectParserInfo parser_info;
    // table key properties.
    PHV::CollectTableKeys tb_keys;
    // physical live ranges of field slices.
    PHV::FieldSliceLiveRangeDB physical_liverange_db;
    // sources of any field slices classified by actions.
    PHV::ActionSourceTracker source_tracker;
    // allocation settings.
    PHV::AllocSetting settings;
    // Collect field packing that table/ixbar would benefit from.
    TableFieldPackOptimization tablePackOpt;

    // a collection class of above passe.
    PHV::AllocUtils utils;

    // Similar as AllocUtils but only used by v2::allocator.
    PHV::v2::PhvKit kit;

    AllocatePHV allocate_phv;
    std::list<const PHV::SuperCluster *> unallocated;

 public:
    PHV_AnalysisPass(const BFN_Options &options, PhvInfo &phv, PhvUse &uses, const ClotInfo &clot,
                     FieldDefUse &defuse, DependencyGraph &deps, const DeparserCopyOpt &decaf,
                     MauBacktracker &alloc, CollectPhvLoggingInfo *phvLoggingInfo,
                     std::set<PHV::FieldRange> &mauInitFields, const TableSummary &table_summary);

    Visitor *make_incremental_alloc_pass(const ordered_set<PHV::Field *> &temp_vars);

    void set_trivial_alloc(bool enable) { settings.trivial_alloc = enable; }
    void set_no_code_change(bool enable) { settings.no_code_change = enable; }
    void set_physical_liverange_overlay(bool enable) {
        settings.physical_liverange_overlay = enable;
    }
    void set_physical_stage_trivial(bool enable) { settings.physical_stage_trivial = enable; }
    const bool &get_limit_tmp_creation() { return settings.limit_tmp_creation; }
    const PHV::Pragmas &get_pragmas() { return pragmas; }
    void end_apply() override;
};

#endif /* BF_P4C_PHV_PHV_ANALYSIS_H_ */
