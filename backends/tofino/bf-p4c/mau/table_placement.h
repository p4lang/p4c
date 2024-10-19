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

/**
 * \defgroup table_placement Table placement
 * \brief Content related to table placement
 *
 * TODO High-level description of table placement
 */

#ifndef BF_P4C_MAU_TABLE_PLACEMENT_H_
#define BF_P4C_MAU_TABLE_PLACEMENT_H_

#include <map>

#include "bf-p4c/backend.h"
#include "bf-p4c/mau/dynamic_dep_metrics.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/table_flow_graph.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/mau/table_summary.h"
#include "lib/ordered_set.h"

using namespace P4;

struct DependencyGraph;
class TablesMutuallyExclusive;
struct StageUseEstimate;
class PhvInfo;
class LayoutChoices;
class SharedIndirectAttachedAnalysis;
class FindPayloadCandidates;

class TablePlacement : public PassManager {
 public:
    const BFN_Options &options;
    DependencyGraph &deps;
    const TablesMutuallyExclusive &mutex;
    PhvInfo &phv;
    LayoutChoices &lc;
    const SharedIndirectAttachedAnalysis &siaa;
    CalculateNextTableProp ntp;
    ControlPathwaysToTable con_paths;
    DynamicDependencyMetrics ddm;
    SplitAttachedInfo &att_info;
    TableSummary &summary;
    MauBacktracker &mau_backtracker;
    bool success = false;

    struct Placed;
    struct RewriteForSplitAttached;
    const TablePlacement::Placed *placement = nullptr;

    struct TableInfo {
        int uid = -1;
        const IR::MAU::Table *table;
        ordered_set<const IR::MAU::TableSeq *> refs;
        bitvec parents;  // Tables that invoke seqs containing this table
        bitvec tables;   // this table and all tables control dependent on it
    };

    struct TableSeqInfo {
        bool root = false;
        int uid = -1;
        bitvec parents;       // same as 'refs', as a bitvec
        bitvec immed_tables;  // the tables directly in the sequence
        bitvec tables;        // the tables in the seqence and their control dependent children
        ordered_set<const IR::MAU::Table *> refs;  // parent tables of this seq
    };

    static int placement_round;
    static bool can_duplicate(const IR::MAU::AttachedMemory *);
    bool can_split(const IR::MAU::Table *, const IR::MAU::AttachedMemory *);

    std::map<const IR::MAU::Table *, struct TableInfo> tblInfo;
    std::vector<struct TableInfo *> tblByUid;
    std::map<cstring, struct TableInfo *> tblByName;
    std::map<const IR::MAU::TableSeq *, struct TableSeqInfo> seqInfo;
    std::map<const IR::MAU::AttachedMemory *, ordered_set<const IR::MAU::Table *>> attached_to;
    std::set<const IR::MAU::Table *> not_eligible;
    int uid(const IR::MAU::Table *t) { return tblInfo.at(t).uid; }
    int uid(cstring t) { return tblByName.at(t)->uid; }
    int uid(const IR::MAU::TableSeq *t) {
        if (seqInfo.count(t) == 0) return -1;
        return seqInfo.at(t).uid;
    }
    const IR::MAU::Table *getTblByName(cstring t) {
        if (auto *ti = get(tblByName, t)) {
            return ti->table;
        }
        LOG5("No tbl info found for table : " << t);
        return nullptr;
    }
    class SetupInfo;

    TablePlacement(const BFN_Options &, DependencyGraph &, const TablesMutuallyExclusive &,
                   PhvInfo &, LayoutChoices &, const SharedIndirectAttachedAnalysis &,
                   SplitAttachedInfo &, TableSummary &, MauBacktracker &);

    struct FinalRerunTablePlacementTrigger : public Backtrack::trigger {
        bool limit_tmp_creation;
        explicit FinalRerunTablePlacementTrigger(bool l)
            : Backtrack::trigger(OK), limit_tmp_creation(l) {}

        DECLARE_TYPEINFO(FinalRerunTablePlacementTrigger);
    };

    using GatewayMergeChoices = ordered_map<const IR::MAU::Table *, cstring>;
    GatewayMergeChoices gateway_merge_choices(const IR::MAU::Table *table);

    typedef enum {
        CALC_STAGE,
        PROV_STAGE,
        NEED_MORE,
        SHARED_TABLES,
        PRIORITY,
        DOWNWARD_PROP_DSC,
        LOCAL_DSC,
        LOCAL_DS,
        LOCAL_TD,
        DOWNWARD_DOM_FRONTIER,
        DOWNWARD_TD,
        NEXT_TABLE_OPEN,
        CDS_PLACEABLE,
        CDS_PLACE_COUNT,
        AVERAGE_CDS_CHAIN,
        DEFAULT
    } choice_t;
    bool backtrack(trigger &) override;

    // tracking reasons tables were passed up for logging.  Currently not backtracking
    // friendly
    struct RejectReason {
        choice_t reason;
        int stage;
        int entries;
        attached_entries_t attached_entries;
    };
    std::map<cstring, std::map<cstring, RejectReason>> rejected_placements;
    void reject_placement(const Placed *of, choice_t reason, const Placed *better);

    // In both in class.  Remove
    std::array<bool, 3> table_in_gress = {{false, false, false}};
    cstring error_message;
    bool ignoreContainerConflicts = false;
    bool limit_tmp_creation = false;
    std::array<const IR::MAU::Table *, 2> starter_pistol = {{nullptr, nullptr}};
    bool alloc_done = false;

    profile_t init_apply(const IR::Node *root) override;
    void end_apply() override { placement_round++; };

    bool try_pick_layout(const gress_t &gress, std::vector<Placed *> tables_to_allocate,
                         std::vector<Placed *> tables_placed);
    bool try_alloc_adb(const gress_t &gress, std::vector<Placed *> tables_to_allocate,
                       std::vector<Placed *> tables_placed);
    bool try_alloc_imem(const gress_t &gress, std::vector<Placed *> tables_to_allocate,
                        std::vector<Placed *> tables_placed);

    bool try_alloc_ixbar(Placed *next, std::vector<Placed *> allocated_layout);
    bool try_alloc_format(Placed *next, bool gw_linked);
    bool try_alloc_mem(Placed *next, std::vector<Placed *> whole_stage);
    void setup_detached_gateway(IR::MAU::Table *tbl, const Placed *placed);
    void filter_layout_options(Placed *pl);
    bool disable_split_layout(const IR::MAU::Table *tbl);
    bool pick_layout_option(Placed *next, std::vector<Placed *> allocated_layout);
    bool shrink_attached_tbl(Placed *next, bool first_time, bool &done_shrink);
    bool shrink_estimate(Placed *next, int &srams_left, int &tcams_left, int min_entries);
    bool shrink_preferred_lo(Placed *next);
    TableSummary::PlacementResult try_alloc_all(Placed *next, std::vector<Placed *> whole_stage,
                                                const char *what, bool no_memory = false);
    bool initial_stage_and_entries(TablePlacement::Placed *rv, int &furthest_stage);
    Placed *try_place_table(Placed *rv, const StageUseEstimate &current,
                            const TableSummary::PlacedTable *pt = nullptr);
    safe_vector<Placed *> try_place_table(const IR::MAU::Table *t, const Placed *done,
                                          const StageUseEstimate &current, GatewayMergeChoices &gmc,
                                          const TableSummary::PlacedTable *pt = nullptr);

    friend std::ostream &operator<<(std::ostream &out, choice_t choice);

    const Placed *add_starter_pistols(const Placed *done, const Placed **best,
                                      const StageUseEstimate &current);

    std::multimap<cstring, const Placed *> table_placed;
    std::multimap<cstring, const Placed *>::const_iterator find_placed(cstring name) const;
    void find_dependency_stages(
        const IR::MAU::Table *tbl,
        std::map<int, ordered_map<const Placed *, DependencyGraph::dependencies_t>> &) const;

    template <class... Args>
    void error(Args... args) {
        auto &ctxt = BaseCompileContext::get();
        auto msg = ctxt.errorReporter().format_message(args...);
        LOG5("    defer error: " << msg);
        summary.addPlacementError(msg);
    }
    int errorCount() const { return P4::errorCount() + summary.placementErrorCount(); }
};

class DecidePlacement : public MauInspector {
    TablePlacement &self;
    typedef TablePlacement::Placed Placed;

 public:
    struct GroupPlace;
    class Backfill;
    class BacktrackPlacement;
    class PlacementScore;
    class ResourceBasedAlloc;
    class FinalPlacement;
#ifdef MULTITHREAD
    class TryPlacedPool;
#endif
    class BacktrackManagement;
    explicit DecidePlacement(TablePlacement &s);

 private:
#ifdef MULTITHREAD
    // Arbitrary number of worker threads that should be exposed as compiler option, e.g. "-j4"
    int jobs = 4;
#endif
    struct save_placement_t;
    std::map<cstring, save_placement_t> saved_placements;
    int backtrack_count = 0;  // number of times backtracked in this pipe
    int MaxBacktracksPerPipe = 32;
    bool resource_mode = false;
    std::map<cstring, std::set<int>> bt_attempts;
    void savePlacement(const Placed *, const ordered_set<const GroupPlace *> &, bool);
    void recomputePartlyPlaced(const Placed *, ordered_set<const IR::MAU::Table *> &);
    std::optional<BacktrackPlacement *> find_previous_placement(const Placed *best, int offset,
                                                                bool local_bt, int process_stage);
    std::optional<BacktrackPlacement *> find_backtrack_point(const Placed *, int, bool);
    bool is_better(const Placed *, const Placed *, TablePlacement::choice_t &);
    int get_control_anti_split_adj_score(const Placed *) const;
    int longBranchTagsNeeded(const Placed *, const ordered_set<const GroupPlace *> &,
                             BacktrackPlacement **);

    void initForPipe(const IR::BFN::Pipe *, ordered_set<const GroupPlace *> &);
    bool preorder(const IR::BFN::Pipe *) override;
    /// @returns true if all the metadata initialization induced dependencies for table @p t are
    /// satisfied, i.e. all the tables that must be placed before table @p t (due to ordering
    /// imposed by the live range shrinking pass) have been placed. @returns false otherwise.
    bool are_metadata_deps_satisfied(const Placed *placed, const IR::MAU::Table *t) const;
    Placed *try_backfill_table(const Placed *done, const IR::MAU::Table *tbl, cstring before);
    bool can_place_with_partly_placed(const IR::MAU::Table *tbl,
                                      const ordered_set<const IR::MAU::Table *> &partly_placed,
                                      const Placed *placed);
    bool gateway_thread_can_start(const IR::MAU::Table *, const Placed *placed);
    IR::MAU::Table *create_starter_table(gress_t gress);
    const Placed *place_table(ordered_set<const GroupPlace *> &work, const Placed *pl);
    template <class... Args>
    void error(Args... args) {
        self.error(args...);
    }
    int errorCount() const { return self.errorCount(); }
    std::pair<bool, const Placed *> alt_table_placement(const IR::BFN::Pipe *pipe);
    const Placed *default_table_placement(const IR::BFN::Pipe *pipe);
};

class TransformTables : public MauTransform {
    TablePlacement &self;
    ordered_set<const IR::MAU::Table *> always_run_actions;  // always run actions to be
                                                             // moved to the top-level TableSeq.  A
                                                             // set so we avoid duplicate references

 public:
    explicit TransformTables(TablePlacement &s) : self(s) {}

 private:
    IR::Node *preorder(IR::MAU::TableSeq *) override;
    IR::Node *postorder(IR::MAU::TableSeq *) override;
    IR::Node *preorder(IR::MAU::Table *) override;
    IR::Node *postorder(IR::MAU::Table *) override;
    IR::Node *preorder(IR::MAU::BackendAttached *) override;
    IR::Node *preorder(IR::BFN::Pipe *pipe) override;
    IR::Node *postorder(IR::BFN::Pipe *pipe) override;
    void merge_match_and_gateway(IR::MAU::Table *tbl, const TablePlacement::Placed *placed,
                                 IR::MAU::Table::Layout &gw_layout);
    IR::MAU::Table *break_up_atcam(IR::MAU::Table *tbl, const TablePlacement::Placed *placed,
                                   int stage_table = -1, IR::MAU::Table **last = nullptr);
    IR::Vector<IR::MAU::Table> *break_up_dleft(IR::MAU::Table *tbl,
                                               const TablePlacement::Placed *placed,
                                               int stage_table = -1);
    void table_set_resources(IR::MAU::Table *tbl, const TableResourceAlloc *res, const int entries);
    template <class... Args>
    void error(Args... args) {
        self.error(args...);
    }
    int errorCount() const { return self.errorCount(); }
};

std::ostream &operator<<(std::ostream &out, TablePlacement::choice_t choice);

class MergeAlwaysRunActions : public PassManager {
    TablePlacement &self;

    using TableFieldSlices = std::map<IR::MAU::Table *, PHV::FieldSlice>;

    struct AlwaysRunKey {
        int stage;
        gress_t gress;

        bool operator<(const AlwaysRunKey &ark) const {
            if (stage != ark.stage) return stage < ark.stage;
            return gress < ark.gress;
        }

        AlwaysRunKey(int s, gress_t g) : stage(s), gress(g) {}
    };

    std::map<AlwaysRunKey, ordered_set<const IR::MAU::Table *>> ar_tables_per_stage;
    std::map<AlwaysRunKey, const IR::MAU::Table *> merge_per_stage;
    std::map<AlwaysRunKey, std::set<int>> merged_ar_minStages;
    ordered_map<const IR::MAU::Table *, std::set<PHV::FieldSlice>> written_fldSlice;
    ordered_map<const IR::MAU::Table *, std::set<PHV::FieldSlice>> read_fldSlice;

    // Keep the original start and end of AllocSlice liveranges that have
    // shifted due to table merging
    typedef std::map<const IR::MAU::Table *, int> premerge_table_stg_t;
    ordered_map<PHV::AllocSlice *, premerge_table_stg_t> premergeLRstart;
    ordered_map<PHV::AllocSlice *, premerge_table_stg_t> premergeLRend;

    bool mergedARAwitNewStage;

    profile_t init_apply(const IR::Node *node) override {
        auto rv = PassManager::init_apply(node);
        ar_tables_per_stage.clear();
        merge_per_stage.clear();
        merged_ar_minStages.clear();
        written_fldSlice.clear();
        read_fldSlice.clear();
        premergeLRstart.clear();
        premergeLRend.clear();
        mergedARAwitNewStage = false;

        // MinSTage status before updating slice liveranges and merged table minStage
        LOG7("MIN STAGE DEPARSER stage: " << self.phv.getDeparserStage());
        LOG7(PhvInfo::reportMinStages());
        LOG7("DG DEPARSER stage: " << (self.deps.max_min_stage + 1));
        LOG7(self.deps);

        return rv;
    }

    class Scan : public MauInspector {
        MergeAlwaysRunActions &self;
        bool preorder(const IR::MAU::Table *) override;
        bool preorder(const IR::MAU::Primitive *) override;
        void end_apply() override;

     public:
        explicit Scan(MergeAlwaysRunActions &s) : self(s) {}
    };

    class Update : public MauTransform {
        MergeAlwaysRunActions &self;
        const IR::MAU::Table *preorder(IR::MAU::Table *) override;
        void end_apply() override;

     public:
        explicit Update(MergeAlwaysRunActions &s) : self(s) {}
    };

    // After merging of ARA tables and updating the dependency graph
    // this class updates the minStage info of tables that have dependence
    // relationship to the merged tables that have changed stage. In
    // addition the liveranges of the affected allocated slices (AllocSlice)
    // are also updated.
    class UpdateAffectedTableMinStage : public MauInspector, public TofinoWriteContext {
        MergeAlwaysRunActions &self;
        // Map affected tables to pair of <old, new> minStages
        ordered_map<const IR::MAU::Table *, std::pair<int, int>> tableMinStageShifts;
        ordered_map<PHV::AllocSlice *, std::pair<int, int>> sliceLRshifts;
        std::map<PHV::AllocSlice *, std::pair<bool, bool>> sliceLRmodifies;

        bool preorder(const IR::MAU::Table *) override;
        bool preorder(const IR::Expression *) override;
        void end_apply() override;

     public:
        explicit UpdateAffectedTableMinStage(MergeAlwaysRunActions &s) : self(s) {}
    };

    const IR::MAU::Table *ar_replacement(int st, gress_t gress) {
        AlwaysRunKey ark(st, gress);
        if (ar_tables_per_stage.count(ark) == 0)
            BUG("MergeAlwaysRunActions cannot find stage of an always run table");
        auto set = ar_tables_per_stage.at(ark);
        return *(set.begin());
    }

    template <class... Args>
    void error(Args... args) {
        self.error(args...);
    }
    int errorCount() const { return self.errorCount(); }

 public:
    explicit MergeAlwaysRunActions(TablePlacement &s) : self(s) {
        addPasses({new Scan(*this), new Update(*this), new FindDependencyGraph(self.phv, self.deps),
                   new UpdateAffectedTableMinStage(*this)});
    }
};

#endif /* BF_P4C_MAU_TABLE_PLACEMENT_H_ */
