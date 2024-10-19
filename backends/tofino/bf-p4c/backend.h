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

#ifndef BF_P4C_BACKEND_H_
#define BF_P4C_BACKEND_H_

#include "bf-p4c-options.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/logging/phv_logging.h"
#include "bf-p4c/mau/finalize_mau_pred_deps_power.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/mau/mau_alloc.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/decaf.h"
#include "bf-p4c/parde/parser_header_sequences.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/utils/live_range_report.h"
#include "ir/ir.h"

class FieldDefUse;
struct CollectPhvLoggingInfo;

namespace BFN {

class Backend : public PassManager {
    BFN_Options options;
    PhvInfo phv;
    PhvUse uses;
    ClotInfo clot;
    DependencyGraph deps;
    FieldDefUse defuse;
    TablesMutuallyExclusive mutex;
    DeparserCopyOpt decaf;
    // Since both phv allocation or table allocation result may change compilation_state,
    // compilation_state is moved from table_summary to backend
    State::state_t compilation_state = State::INITIAL;
    /// Class that represents the resource allocation post table placement round
    TableSummary table_summary;
    /// Class that represents the backtracking point from table placement to PHV allocation.
    MauBacktracker mau_backtracker;
    TableAllocPass table_alloc;
    // Primitives Json Node, is populated before instruction adjustment and
    // passed to AsmOutput to output primitive json file
    Util::JsonObject primNode;
    // Dependency Flow Graph Json, is a collection of graphs populated when
    // passed as a parameter to FindDependencyGraph pass. By default graphs are
    // generated once before and once after table placement.
    Util::JsonObject jsonGraph;

    LogFlexiblePacking *flexibleLogging;
    CollectPhvLoggingInfo *phvLoggingInfo;
    PhvLogging::CollectDefUseInfo *phvLoggingDefUseInfo;
    DynamicNextTable nextTblProp;
    MauPower::FinalizeMauPredDepsPower *power_and_mpr;
    LiveRangeReport *liveRangeReport;

    // Identify the header sequences extracted in the parser
    ParserHeaderSequences parserHeaderSeqs;
    // Disable long branches
    bool longBranchDisabled;
    // Fields which are zero initialized in the program and have assignments being eliminated. These
    // fields should have the POV bit set and therefore need to be tracked.
    std::set<cstring> zeroInitFields;
    // Fields that need explicit MAU initialization
    std::set<PHV::FieldRange> mauInitFields;

 protected:
    profile_t init_apply(const IR::Node *root) override {
        BFNContext::get().setBackendOptions(&options);
        PhvInfo::resetDarkSpillARA();
        return PassManager::init_apply(root);
    }

    void end_apply() override { BFNContext::get().clearBackendOptions(); }

 public:
    explicit Backend(const BFN_Options &options, int pipe_id);

    BFN_Options &get_options() { return options; }

    const PhvInfo &get_phv() const { return phv; }
    const ClotInfo &get_clot() const { return clot; }
    const FieldDefUse &get_defuse() const { return defuse; }
    const MauPower::FinalizeMauPredDepsPower *get_power_and_mpr() const { return power_and_mpr; }
    const NextTable *get_nxt_tbl() const { return &nextTblProp; }
    const TableSummary &get_tbl_summary() const { return table_summary; }
    const LiveRangeReport *get_live_range_report() const { return liveRangeReport; }
    const Util::JsonObject &get_prim_json() const { return primNode; }
    const Util::JsonObject &get_json_graph() const { return jsonGraph; }
    const LogRepackedHeaders *get_flexible_logging() const {
        return flexibleLogging->get_flexible_logging();
    }
    const CollectPhvLoggingInfo *get_phv_logging() const { return phvLoggingInfo; }
    const PhvLogging::CollectDefUseInfo *get_phv_logging_defuse_info() const {
        BUG_CHECK(phvLoggingDefUseInfo, "DefUse info for PHV logging was not initialized");
        return phvLoggingDefUseInfo;
    }
    const ordered_map<cstring, ordered_set<int>> &get_table_alloc() const {
        return table_summary.getTableAlloc();
    }
    const ParserHeaderSequences &get_parser_hdr_seqs() const { return parserHeaderSeqs; }
};

}  // namespace BFN

#endif /* BF_P4C_BACKEND_H_ */
