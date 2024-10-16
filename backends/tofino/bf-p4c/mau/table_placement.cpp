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

#include "bf-p4c/mau/table_placement.h"

#ifdef MULTITHREAD
#include <gc/gc.h>
#include <pthread.h>
#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#endif
#include <algorithm>
#include <list>
#include <sstream>
#include <unordered_map>

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/ir/table_tree.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/logging/manifest.h"
#include "bf-p4c/mau/action_data_bus.h"
#include "bf-p4c/mau/attached_entries.h"
#include "bf-p4c/mau/field_use.h"
#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/mau/instruction_memory.h"
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_injected_deps.h"
#include "bf-p4c/mau/table_layout.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/phv_analysis.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "lib/bitops.h"
#include "lib/bitvec.h"
#include "lib/log.h"
#include "lib/pointer_wrapper.h"
#include "lib/safe_vector.h"
#include "lib/set.h"

#ifndef LOGGING_FEATURE
// FIXME -- should be in p4c code in log.h -- remove this once it is added there
#define LOGGING_FEATURE(TAG, N) ((N) <= MAX_LOGGING_LEVEL && ::Log::fileLogLevelIsAtLeast(TAG, N))
#endif

/*! \file table_placement.cpp
 **
 ** Table placement is done using different heuristics to optimize MAU usage under different
 ** use cases. The two main heuristics used are:
 **
 ** 1. Always prioritize dependencies chain.
 ** 2. Balance between dependencies and resource usage.
 **
 ** These heuristics also support backtracking to try to fix a given solution when during the
 ** placement an impossible dependency chain is discovered. Table placement can also be
 ** initiated from the traditional compilation steps (PHV First) or with the alternative
 ** mode which use table placement as the first step. Both compilation mode use table
 ** placement algorithm in a similar way with some exceptions described below.
 **
 ** All of the decisions for placement are done in the preorder(IR::BFN::Pipe *) method --
 ** in this method we go over all the tables in the pipe (directly, not using the visitor
 ** infrastructure) making decisions about which tables should be allocated to which logical
 ** tables and what ixbar an memory resources to use for them.  All of these decisions are
 ** stored in Placed objects (a linked list with one Placed object per logical table) that
 ** is built up in a write-once fashion to allow for backtracking (we can at any time throw
 ** away nodes from the front of the list, backing up and continuing from an earlier point).
 **
 ** After preorder(Pipe) method completes, the transform visits all of the tables in the
 ** pipeline, rewriting them to match the placement decisions made in the Placed list.  This
 ** involves actually combining gateways into match tables and splitting tables across
 ** multiple logical tables as decided in the information recorded in the Placed list.
 **
 ** The placement process itself is a fairly standard greedy allocator -- we maintain a
 ** work list (called 'work' throughout the code) of TableSeq objects from which the next
 ** table can be chosen.  We look at all the tables in the work list and, for each table
 ** that can be placed next, we create a new 'Placed' object linked onto the front of the
 ** 'done' list for that placement.  This may involve revisiting the specific resources used
 ** for tables already placed in this stage, though not the logical table assignments.
 ** Once these possible placements have all been created, we compare them with a variety
 ** of metrics encapsulated in the 'is_better' method to determine which is the best choice --
 ** we keep that (one) Placed and repeat, placing additional tables.
 **
 ** One complex part of this process is maintaining the 'work' list.  Because of the limits
 ** of Tofino1 next table processing, when a table A is assigned to a specific logical table,
 ** all tables that are control dependent on A must be assigned to logical tables before any
 ** table after A in the parent control flow.  That means when we place A, and add all control
 ** dependent TableSeqs of A to the work list, we (temporarily) remove the seq containing A
 ** from the work list.  We record that removed seq as the 'parent' of the added seqs, so that
 ** we can add it back once those children are all placed.  These relationships are recorded
 ** in GroupPlace objects (which are what is actually in the work list) -- one for each
 ** IR::MAU::TableSeq being processed.
 **
 ** For Tofino2, we don't have the above restriction, so we don't remove the parent
 ** GroupPlace from the worklist, but we still maintain the parent/child info even though it
 ** isn't really needed (minimizing the difference between Tofino1 and Tofino2 here).
 **
 ** Traditional compilation mode
 ** ============================
 **
 ** In traditional compilation mode, steps are normally looking like:
 **
 ** 1. PHV Allocation based on parser/deparser/pragmas/actions constraints.
 **
 ** 2. Table Placement using this latest PHV Allocation that typically does not fit since
 **    PHV Allocation was not taking care of any potential container conflicts.
 **
 ** 3. Table Placement using this latest PHV Allocation that ignore container conflict
 **    which ultimately gives an idea of an ideal placement.
 **
 ** 4. PHV Allocation based on parser/deparser/pragmas/actions constraints plus the step 3
 **    placement to avoid future container conflict by not sharing containers for fields
 **    written by two different tables on the same stage.
 **
 ** 5. Table Placement using this latest PHV Allocation that ideally result in a similar
 **    placement as in step 3. This is typically where most program find a solution. In some
 **    cases it requires another iteration of steps 3,4,5. In this case, the PHV Allocation
 **    for the next iteration will uses the first and the second table placement that ignore
 **    the container conflict to relaxes constraint for the final table placement step.
 **
 ** Alternative PHV Allocation compilation mode
 ** ===========================================
 **
 ** With the alternative PHV allocation compilation mode, steps are a bit different and look like
 ** this from high level perspective:
 **
 ** 1. Trivial PHV Allocation based on parser/deparser/pragmas/actions constraints using an
 **    infinite number of containers of all types. Most of the fields are carried through
 **    individual containers. The only case where fields packing is involved is when it would
 **    benefit table matching by reducing IxBar and SRAMs/TCAMs consumption. This PHV Allocation
 **    can not be synthesize.
 **
 ** 2. Table Placement using this latest PHV Allocation should converge with a solution in this
 **    early stage since the PHV constraints are minimal. Unfortunately, even if table placement
 **    find a solution, this latter one cannot be used as a final solution since it involve
 **    a PHV allocation that can not be synthesize.
 **
 ** 3. PHV Allocation based on parser/deparser/pragmas/actions constraints plus the previous
 **    table placement run to avoid container conflict and also have a better understanding of the
 **    physical live range of every field slices.
 **
 ** 4. Table Placement using basic replay that assume it can allocate the same table as in step
 **    2 with the same ordering and the same number of entries per table parts (for large table
 **    that spans on multiple stages). The only difference is that it now uses the real PHV
 **    allocation of step 3 which make this placement a final one if it successful. Unfortunately
 **    it is possible that it fails to replay the placement because of added IxBar/Match/Actions
 **    constraints caused by different container size allocated vs the trivial PHV allocation
 **    step.
 **
 ** 5. Table Placement is involved in case it fails to do the replay (step 4) by trying a normal
 **    allocation using various heuristics. Dependencies are injected before this allocation can
 **    take place to account for the various fields/containers overlay introduced by the latest
 **    PHV allocation.
 **
 ** Table Placement heuristics details
 ** ==================================
 **
 ** Two main heuristics are used for table placement. Originally only the one that always
 ** prioritize dependencies chain was in place but that heuristic was not good for program that
 ** uses resources heavily. Only looking for downward dependencies make sure that your critical
 ** path is covered but this can be at the expense of low MAU resource utilization. P4 developers
 ** had to uses multiple pragmas to steer the table placement in some direction and have a better
 ** resources utilization. This was annoying for the developers but also for the compiler engineer
 ** that was always trying to modify current P4 program every time basic heuristics changed. After
 ** some P4 programs example from various customer we were able to find two main types of programs
 ** that have different characteristic and would be better managed by two different types of
 ** placement heuristics.
 **
 ** 1. Programs with multiple conditions but with modest table size. These programs require lots
 **    of table ids, have big dependency chains but does not consume many SRAMs, TCAMs and map
 **    resources. These kind of programs was well handled by the original heuristic that always
 **    favors the longest downward dependencies.
 **
 ** 2. Programs with lots of big tables that consume up to 80% of the total MAU resources for
 **    the whole pipeline. These programs can also have deep dependencies but typically they
 **    are smaller because chains of conditions are translated into single table lookup most
 **    of the time.
 **
 ** The first type of program was better handled by the original heuristic but for the second one
 ** every modification in the compiler heuristics regarding PHVs, table layout, initialization,
 ** placement optimization, ... resulted in multiple fitting issues.
 **
 ** Always prioritize dependencies chain
 ** ------------------------------------
 **
 ** This was the original approach which basically look at the downward dependencies of all
 ** the eligible tables to be placed and select the one with the longest. This behavior can
 ** be steered by using pragmas to force table in specific stages. Other pragmas can also be
 ** used to increase or reduce the priority of tables to force a selection even if this table
 ** is not the one with the longest downward dependencies at a given moment. This basic heuristic
 ** look at all eligible table, try to add each of them one by one and compare the result through
 ** the 'is_better' function (which basically only look at downward dependencies)
 **
 ** Balance between dependencies and resource usage
 ** -----------------------------------------------
 **
 ** This heuristic was added as a second strategy to try when the original heuristic fails to
 ** find a solution. This strategy is really good at finding solution for the program that
 ** require lots of resources. This strategy is always run after the original approach because
 ** it uses the result of that first allocation to build the total program resource usage.
 ** As of now this heuristic tracks TCAMs, SRAMs, Table Logical Ids and Map RAM. The first step
 ** is to compute the total number of resource element required for the entire program to be used
 ** in the decision process for priorization of the most heavily resources required. The second
 ** step is to proceed with a similar table placement heuristic as the original one but with the
 ** exception that the idea is to build multiple solutions for a given MAU stage that can be
 ** compared and selected based on a combination of resource and dependencies criteria.
 ** Unfortunately, it is not efficient to build all the possible solutions for every stages since
 ** the number of solutions will increase exponentially based on the number of eligible
 ** tables. The trade-off used in this situation is to make sure the algorithm has at least one
 ** solution that includes every table that got eligible at any given point. e.g.:
 **
 ** Stage **X** have eligible Table *A*, *B*, *C*, *D* when empty.
 **
 ** 1. Place Table *A* based on heuristic.
 ** 2. *B*, *D*, *E* are now eligible (**-C**, **+E**).
 ** 3. Place Table D based on heuristic.
 ** 4. No Other table Fit on Stage **X** (Solution include Table *A* and *D*).
 ** 5. Found that Table *B*, *C*, *E* was eligible at some point but not included in any solution.
 ** 6. Go Back and build a new solution by selecting Table *B* Originally.
 ** 7. *C*, *D*, *F* are now eligible (**-A**, **+F**).
 ** 8. Place Table *D* based on heuristic.
 ** 9. *F* is now eligible (**-C**).
 ** 10. Place Table *F*.
 ** 11. No Other table Fit on Stage **X** (Solution include Table *B*, *D* and *F*).
 ** 12. Continue until it finds at least one solution that includes Table *C* and *E* and possibly
 **     other since new table can become eligible after being unblocked by another one.
 ** 13. Compare the solutions and select the best one based on resource and dependency metrics.
 ** 14. Move to the next stage and proceed with the same steps.
 **
 ** Selecting the best solution
 ** ---------------------------
 **
 ** The second heuristic must select one of the solutions for a given stage before moving to the
 ** next one. This selection is driven by a combination of dependencies and resources. It first
 ** scans all the solutions to find the longest downward dependency than exclude all of the
 ** solutions that does not include the maximum number of tables with this longest downward
 ** dependency value. The remaining solutions will be evaluated from resource perspective by
 ** giving a score to each of them. The score is based on the program resource requirement to
 ** select the best solution. For example, a program that require lots of SRAM resource but not
 ** many TCAM resources will give better score to solutions with high SRAM vs TCAM usage. The
 ** resource is also dynamically track during the placement, so it is possible that at some point
 ** a resource become more critical if previous stages did not consume enough of it.
 **
 ** Backtracking
 ** ------------
 **
 ** Backtracking mechanism was added in table placement as another strategy to find solutions and
 ** effectively move out from low minima. Backtracking is supported on both table placement
 ** heuristics and is cap to a maximum number of attempts. That maximum number is different between
 ** the steps to balance the compilation time with the usefulness of backtracking. For example,
 ** that limit is significantly lower on no container conflict because at this stage we know that
 ** another table placement step will be required. The backtracking mechanism kicks in when the
 ** remaining downward dependencies lead to impossible placement from a mathematical perspective.
 ** This is computed after each table selection and compared to the number of stages remaining in
 ** the target. In this case, the backtracking mechanism try to find a way to move that
 ** problematic table in a stage that would cover for the computed downward dependency. Most of
 ** the time that is not possible because a whole dependency chain needs to be moved. Therefore
 ** the backtracking mechanism will try to move back in earlier stages to try to move a table of
 ** that dependency chain to ultimately move the entire chain at least one stage earlier.
 **
 ** Backtrack Placement
 ** -------------------
 **
 ** The backtracking mechanism require specific saved position to go back into that state. Every
 ** eligible table that was tried during table placement can be saved as a potential backtracking
 ** position. Unfortunately doing that would require an excessive amount of memory and the
 ** backtracking mechanism would not be able to visit all of them. The backtracking mechanism
 ** currently only saves two positions for each table and stage tuple. These two positions are
 ** named local and global. The global position refer to the earliest backtracking point of a
 ** table on each stages it was eligible across all backtracking attempts. The local position
 ** refers to the earliest backtracking point of a table on each stage it was eligible since the
 ** last backtracking attempt. The backtracking mechanism will always try to relax the actual
 ** constraint using local position to try to increase the solution as much as possible instead
 ** of jumping to potentially a really different solution from the global point. Backtrack point
 ** will not be saved if the selected table have high priority or stage pragma to make sure
 ** backtracking does not override such customer request.
 **
 ** Table Placement Heuristics Navigation
 ** -------------------------------------
 **
 ** The previously described heuristics are not always tried on all steps because in some cases
 ** it is not required. Table Placement currently only care about being able to fit all of the
 ** tables in the required number of stages. Multiple heuristics can be tried to achieve this
 ** goal but when a solution is found, the algorithm will accept that solution and stop looking
 ** for alternative way. The typical sequence being evaluated by the table placement is:
 **
 ** 1. Table selected by dependency only.
 ** 2. Table selected by dependency only with backtracking.
 ** 3. Table selected by a combination of resources tracking + dependency.
 ** 4. Table selected by a combination of resources tracking + dependency with backtracking.
 **
 ** The table placement for each strategy will stop and be saved as an incomplete placement if at
 ** some point, the algorithm detect that downward dependency requirement is impossible to reach.
 ** When such incomplete placement is saved, the next strategy is selected. If none of these
 ** strategies is found to produce a placement that fit the target, all of the incomplete
 ** placement will be finalized and compared. The one that require the least stages will be
 ** selected as the final one. The first strategy are prioritize over the last if multiple
 ** strategy require the same number of stages.
 */

int TablePlacement::placement_round = 1;

bool TablePlacement::backtrack(trigger &trig) {
    // If a table does not fit in the available stages, then TableSummary throws an exception.
    // TablePlacement catches that exception and re-runs table placement without considering
    // container conflicts. This gives the "idealized" resource-sensitive table placement that is
    // then used by PHV allocation to minimize the number of container conflicts seen in the final
    // allocation.
    if (trig.is<RerunTablePlacementTrigger::failure>()) {
        auto t = static_cast<RerunTablePlacementTrigger::failure *>(&trig);
        ignoreContainerConflicts = t->ignoreContainerConflicts;
        return true; }
    ignoreContainerConflicts = false;
    // If temporary variable was added during placement we have two possibilities:
    // 1 - PHV was able to allocate these variable incrementally and a new table allocation run is
    //     needed to properly fill the actions that make uses of these temporary metadata field.
    //
    // 2 - PHV was not able to allocate these variable. In this case, we try to re-run table
    //     allocation by disabling features that create temporary variable during placement and
    //     hope that a solution would be found. Currently the only feature disabled is the split
    //     attach that carry indexes and flag from stage to stage.
    if (trig.is<FinalRerunTablePlacementTrigger>()) {
        auto t = dynamic_cast<FinalRerunTablePlacementTrigger *>(&trig);
        limit_tmp_creation = t->limit_tmp_creation;
        if (!limit_tmp_creation)
            summary.FinalizePlacement();
        else
            summary.setPrevState();
        return true; }
    return false;
}

Visitor::profile_t TablePlacement::init_apply(const IR::Node *root) {
    auto rv = PassManager::init_apply(root);
    alloc_done = phv.alloc_done();
    summary.clearPlacementErrors();
    LOG1("Table Placement " << summary.getActualStateStr() <<
         (ignoreContainerConflicts ? "," : ", not") << " ignoring container conflicts");
    if (BackendOptions().create_graphs) {
        static unsigned invocation = 0;
        auto pipeId = root->to<IR::BFN::Pipe>()->canon_id();
        auto graphsDir = BFNContext::get().getOutputDirectory("graphs"_cs, pipeId);
        cstring fileName = "table_dep_graph_placement_"_cs + std::to_string(invocation++);
        std::ofstream dotStream(graphsDir + "/" + fileName + ".dot", std::ios_base::out);
        DependencyGraph::dump_viz(dotStream, deps);
        Logging::Manifest::getManifest().addGraph(pipeId, cstring("table"), fileName,
                                                  INGRESS);  // this should be both really!
    }
    return rv;
}

class TablePlacement::SetupInfo : public Inspector {
    TablePlacement &self;
    bool preorder(const IR::MAU::Table *tbl) override {
        BUG_CHECK(!self.tblInfo.count(tbl), "Table in both ingress and egress?");
        BUG_CHECK(self.tblInfo.size() == self.tblByUid.size(), "size mismatch in SetupInfo");
        auto &info = self.tblInfo[tbl];
        info.uid = self.tblByUid.size();
        info.table = tbl;
        self.tblByUid.push_back(&info);
        auto *seq = getParent<IR::MAU::TableSeq>();
        BUG_CHECK(seq, "parent of Table is not TableSeq");
        info.refs.insert(seq);
        BUG_CHECK(!self.tblByName.count(tbl->name), "Duplicate tables named %s: %s and %s",
                  tbl->name, tbl, self.tblByName.at(tbl->name)->table);
        self.tblByName[tbl->name] = &info;
        return true; }
    void revisit(const IR::MAU::Table *tbl) override {
        auto *seq = getParent<IR::MAU::TableSeq>();
        BUG_CHECK(seq, "parent of Table is not TableSeq");
        self.tblInfo.at(tbl).refs.insert(seq); }
    void postorder(const IR::MAU::Table *tbl) override {
        auto &info = self.tblInfo.at(tbl);
        info.tables[info.uid] = 1;
        for (auto &n : tbl->next)
            info.tables |= self.seqInfo.at(n.second).tables; }
    bool preorder(const IR::MAU::BackendAttached *ba) override {
        visitAgain();
        BUG_CHECK(getParent<IR::MAU::Table>(), "parent of BackendAttached is not Table");
        self.attached_to[ba->attached].insert(getParent<IR::MAU::Table>());
        return false; }
    bool preorder(const IR::MAU::TableSeq *seq) override {
        BUG_CHECK(!self.seqInfo.count(seq), "TableSeq in both ingress and egress?");
        auto &info = self.seqInfo[seq];
        info.uid = self.seqInfo.size() - 1;
        if (auto tbl = getParent<IR::MAU::Table>()) {
            info.refs.insert(tbl);
        } else if (getParent<IR::BFN::Pipe>()) {
            info.root = true;
        } else {
            BUG("parent of TableSeq is not a table or pipe");
        }
        return true; }
    void revisit(const IR::MAU::TableSeq *seq) override {
        BUG_CHECK(self.seqInfo.count(seq), "TableSeq not present");
        BUG_CHECK(!self.seqInfo[seq].refs.empty(), "TableSeq is root and non-root");
        auto tbl = getParent<IR::MAU::Table>();
        BUG_CHECK(tbl, "parent of TableSeq is not a table");
        self.seqInfo[seq].refs.insert(tbl); }
    void postorder(const IR::MAU::TableSeq *seq) override {
        auto &info = self.seqInfo.at(seq);
        for (auto t : seq->tables) {
            info.immed_tables[self.uid(t)] = 1;
            info.tables |= self.tblInfo.at(t).tables; } }
    bool preorder(const IR::MAU::Action *) override { return false; }
    bool preorder(const IR::Expression *) override { return false; }
    bool preorder(const IR::V1Table *) override { return false; }

    profile_t init_apply(const IR::Node *node) override {
        auto rv = Inspector::init_apply(node);
        self.tblInfo.clear();
        self.tblByUid.clear();
        self.tblByName.clear();
        self.seqInfo.clear();
        self.attached_to.clear();
        return rv;
    }

    void end_apply() override {
        for (auto &att : self.attached_to) {
            LOG7("Attached memory:" << att.first->name);
            for (auto tbl : att.second)
                LOG7("   --> tbl:" << tbl->name);
            if (att.second.size() == 1) continue;
            if (att.first->direct)
                self.error("direct %s attached to multiple match tables", att.first);
        }
        for (auto &seq : Values(self.seqInfo))
            for (auto *tbl : seq.refs)
                seq.parents.setbit(self.tblInfo.at(tbl).uid);
        for (auto &tbl : Values(self.tblInfo))
            for (auto *seq : tbl.refs)
                tbl.parents |= self.seqInfo.at(seq).parents;
    }

 public:
    explicit SetupInfo(TablePlacement &self_) : self(self_) {}
};


struct DecidePlacement::GroupPlace {
    /* tracking the placement of a group of tables from an IR::MAU::TableSeq
     *   parents    groups that must wait until this group is fully placed before any more
     *              tables from them may be placed (so next_table setup works)
     *   ancestors  union of parents and all parent's ancestors
     *   seq        the TableSeq being placed for this group */
    const DecidePlacement                &self;
    ordered_set<const GroupPlace *>     parents, ancestors;
    const IR::MAU::TableSeq             *seq;
    const TablePlacement::TableSeqInfo  &info;
    int                                 depth;  // just for debugging?
    GroupPlace(const DecidePlacement &self_, ordered_set<const GroupPlace*> &work,
               const ordered_set<const GroupPlace *> &par, const IR::MAU::TableSeq *s)
    : self(self_), parents(par), ancestors(par), seq(s), info(self.self.seqInfo.at(s)), depth(1) {
        for (auto p : parents) {
            if (depth <= p->depth)
                depth = p->depth+1;
            ancestors |= p->ancestors; }
        LOG4("    new seq " << seq->clone_id << " depth=" << depth << " anc=" << ancestors);
        work.insert(this);
        if (Device::numLongBranchTags() == 0 || self.self.options.disable_long_branch) {
            // Table run only with next_table, so can't continue placing ancestors until
            // this group is finished
            if (LOGGING(5)) {
                for (auto a : ancestors)
                    if (work.count(a))
                        LOG5("      removing ancestor " << a->seq->clone_id << " from work list"); }
            work -= ancestors; } }

    /// finish a table group -- remove it from the work queue and append its parents
    /// unless the parent or a descendant is already present in the queue.
    /// @returns an iterator to the newly added groups, if any.
    ordered_set<const GroupPlace *>::iterator finish(ordered_set<const GroupPlace *> &work) const {
        auto rv = work.end();
        LOG5("      removing " << seq->clone_id << " from work list (a)");
        work.erase(this);
        for (auto p : parents) {
            if (work.count(p)) continue;
            bool skip = false;
            for (auto *gp : work) {
                if (gp->ancestors.count(p)) {
                    skip = true;
                    break; } }
            if (skip) continue;
            LOG5("      appending " << p->seq->clone_id << " to work queue");
            auto t = work.insert(p);
            if (rv == work.end()) rv = t.first; }
        return rv; }
    void finish_if_placed(ordered_set<const GroupPlace*> &, const Placed *) const;
    static bool in_work(ordered_set<const GroupPlace*> &work, const IR::MAU::TableSeq *s) {
        for (auto pl : work)
            if (pl->seq == s) return true;
        return false; }
    friend std::ostream &operator<<(std::ostream &out,
        const ordered_set<const DecidePlacement::GroupPlace *> &set) {
        out << "[";
        const char *sep = " ";
        for (auto grp : set) {
            out << sep << grp->seq->clone_id;
            sep = ", "; }
        out << (sep+1) << "]";
        return out; }
};

struct TablePlacement::Placed {
    /* A linked list of table placement decisions, from last table placed to first (so we
     * can backtrack and create different lists as needed) */
    TablePlacement              &self;
    const int                   id, clone_id;
    const Placed                *prev = 0;
    const DecidePlacement::GroupPlace *group = 0;  // work group chosen from
    cstring                     name;
    int                         entries = 0;
    int                         requested_stage_entries = -1;  // pragma stage requested entries
    int                         stage_flags = 0;  // pragma stage flags
    attached_entries_t          attached_entries;
    bitvec                      placed;  // fully placed tables after this placement
    bitvec                      match_placed;  // tables where the match table is fully placed,
                                               // but indirect attached tables may not be
    int                         complete_shared = 0;  // tables that share attached tables and
                                        // are completely placed by placing this
    cstring                     stage_advance_log;  // why placement had to go to next stage

    /// True if the table needs to be split across multiple stages, because it
    /// can't fit within a single stage (eg. not enough entries in the stage).
    bool                        need_more = false;
    /// True if the match table (only) needs to be split across stages
    bool                        need_more_match = false;
    // the above two flags are redundant with info in 'placed' and 'match_placed', so could
    // be eliminated and uses replaced by checks of those bitvecs

    cstring                     gw_result_tag;
    const IR::MAU::Table        *table, *gw = 0;
    int                         stage = 0, logical_id = -1;
    /// Information on which stage table this table is associated with.  If the table is
    /// never split, then the stage_split should be -1
    int                         stage_split = -1;
    StageUseEstimate            use;
    TableResourceAlloc          resources;
    Placed(TablePlacement &self_, const IR::MAU::Table *t, const Placed *done)
        : self(self_), id(getNextUid()), clone_id(id), prev(done), table(t) {
        if (t) { name = t->name; }
        if (done) {
            stage = done->stage;
            placed = done->placed;
            match_placed = done->match_placed; }
        traceCreation();
    }

    // test if this table is placed
    bool is_placed(const IR::MAU::Table *tbl) const {
        return placed[self.uid(tbl)]; }
    bool is_placed(cstring tbl) const {
        return placed[self.uid(tbl)]; }
    bool is_match_placed(const IR::MAU::Table *tbl) const {
        return match_placed[self.uid(tbl)]; }
    // test if this table or seq and all its control dependent tables are placed
    bool is_fully_placed(const IR::MAU::Table *tbl) const {
        return placed.contains(self.tblInfo.at(tbl).tables); }
    bool is_fully_placed(const IR::MAU::TableSeq *seq) const {
        return placed.contains(self.seqInfo.at(seq).tables); }
    const DecidePlacement::GroupPlace *find_group(const IR::MAU::Table *tbl) const {
        for (auto p = this; p; p = p->prev) {
            if (p->table == tbl || p->gw == tbl)
                return p->group; }
        BUG("Can't find group for %s", tbl->name);
        return nullptr; }

    void gateway_merge(const IR::MAU::Table*, cstring);

    /// Update a Placed object to reflect attached tables being allocated in the same stage due
    /// to another table being added to the stage.
    void update_attached(Placed *latest) {
        if (need_more && !need_more_match) {
            need_more = false;
            for (auto *ba : table->attached) {
                if (ba->attached->direct) continue;
                BUG_CHECK(attached_entries.count(ba->attached),
                          "initial_stage_and_entries mismatch for %s", ba->attached);
                auto it = latest->attached_entries.find(ba->attached);
                if (it != latest->attached_entries.end()) {
                    BUG_CHECK(attached_entries.at(ba->attached).entries == 0 ||
                              attached_entries.at(ba->attached).entries == it->second.entries,
                              "inconsistent size for %s", ba->attached);
                    attached_entries.at(ba->attached) = it->second;
                    use.format_type.invalidate(); }
                if (attached_entries.at(ba->attached).need_more)
                    need_more = true; }
            if (!need_more) {
                LOG3("    " << table->name << " is now also placed (1)");
                latest->complete_shared++;
                placed[self.uid(table)] = 1; } }
        if (prev)
            placed |= prev->placed; }

    void update_need_more(int needed_entries) {
        if (entries < needed_entries) {
            need_more = need_more_match = true;
        } else {
            need_more = need_more_match = false;
            for (auto *ba : table->attached) {
                if (!ba->attached->direct && attached_entries.at(ba->attached).need_more) {
                    need_more = true;
                    break;
                }
            }
        }
        LOG3("Entries : " << entries << ", needed_entries: " << needed_entries
                          << ", need_more: " << need_more);
    }

#if 0
    // now unused -- maybe rotted
    // update the 'placed' bitvec to reflect tables that were previously partly placed and
    // are now fully placed.
    void update_for_partly_placed(const ordered_set<const IR::MAU::Table *> &partly_placed) {
        for (auto *pp : partly_placed) {
            if (placed[self.uid(pp)]) continue;  // already done
            if (!match_placed[self.uid(pp)]) continue;  // not yet done match
            bool check = false;
            // don't bother to recheck tables for which we have not added any attached entries
            for (auto *ba : pp->attached) {
                if (ba->attached->direct) continue;
                if (attached_entries.count(ba->attached)) {
                    check = true;
                    break; } }
            if (!check) continue;
            bool need_more = false;
            for (auto *ba : pp->attached) {
                if (ba->attached->direct) continue;
                if (attached_entries.at(ba->attached).need_more) {
                    need_more = true;
                    break; } }
            if (!need_more) {
                LOG3("    " << pp->name << " is now also placed (2)");
                complete_shared++;
                placed[self.uid(pp)] = 1; } } }
#endif

    // update the action/meter formats in the TableResourceAlloc to match the StageUseEstimate
    void update_formats() {
        if (auto *af = use.preferred_action_format())
            resources.action_format = *af;
        if (auto *mf = use.preferred_meter_format())
            resources.meter_format = *mf; }

    // how many logical ids are needed for a placed object
    int logical_ids() const {
        if (table->is_always_run_action()) return 0;
        if (table->conditional_gateway_only()) return 1;
        return use.preferred()->logical_tables();
    }

    void setup_logical_id() {
        LOG7("\t\tSetting logical id for table " << table->externalName());
        if (table->is_always_run_action()) {
            logical_id = -1;
            return;
        }

        auto curr = prev;
        for (; curr; curr = curr->prev) {
            if (curr->table->is_always_run_action()) continue;
            if (!Device::threadsSharePipe(table->gress, curr->table->gress)) continue;
            break;
        }

        if (curr && curr->stage == stage) {
            logical_id = curr->logical_id + curr->logical_ids();
        } else {
            logical_id = stage * StageUse::MAX_LOGICAL_IDS;
        }
        LOG7("\t\t\tLogical ID: " << logical_id);
    }

    // how many logical id slots are left in the current stage?
    int logical_ids_left() const {
        auto *p = this;
        for (; p; p = p->prev) {
            if (p->table->is_always_run_action()) continue;
            if (!Device::threadsSharePipe(table->gress, p->table->gress)) continue;
            break; }
        if (!p || p->stage != stage)
            return StageUse::MAX_LOGICAL_IDS;
        int left = StageUse::MAX_LOGICAL_IDS - (p->logical_id % StageUse::MAX_LOGICAL_IDS);
        left -= p->logical_ids();
        return left;
    }

    friend std::ostream &operator<<(std::ostream &out, const Placed *pl) {
        out << pl->name;
        return out; }

    friend std::ostream &operator<<(std::ostream &out, const std::vector<Placed *> &placed_vector) {
        for (auto &p : placed_vector)
            out << p << " ";
        return out;
    }

    Placed(const Placed &p)
        : self(p.self), id(getNextUid()), clone_id(p.clone_id), prev(p.prev), group(p.group),
           name(p.name), entries(p.entries), requested_stage_entries(p.requested_stage_entries),
          attached_entries(p.attached_entries), placed(p.placed),
          match_placed(p.match_placed), complete_shared(p.complete_shared),
          stage_advance_log(p.stage_advance_log),
          need_more(p.need_more), need_more_match(p.need_more_match),
          gw_result_tag(p.gw_result_tag), table(p.table), gw(p.gw), stage(p.stage),
          logical_id(p.logical_id),
          stage_split(p.stage_split), use(p.use), resources(p.resources) { traceCreation(); }

    const Placed *diff_prev(const Placed *new_prev) const {
        auto rv = new Placed(*this);
        rv->prev = new_prev;
        return rv;
    }

    // Find initial position of a table in case this one is split across multiple stages
    int init_stage() const {
        const IR::MAU::Table *search_tbl = table;
        const Placed *prev_pl = prev;
        int first_stage = stage;
        while (prev_pl) {
            if (prev_pl->table == search_tbl)
                first_stage = prev_pl->stage;
            prev_pl = prev_pl->prev;
        }
        return first_stage;
    }

 private:
    Placed(Placed &&) = delete;
    void traceCreation() { }
    int getNextUid() {
#ifdef MULTITHREAD
        static std::atomic<int> uid_counter(0);
#else
        static int uid_counter = 0;
#endif
        return ++uid_counter;
    }
};

// Retrieve the stage resources for a given placement
// Optional argument accumulate_low_pri that can be set to false to avoid accumulating low
// priority table for the returned status. This is mainly used in resource based allocation
// to make sure the compared solution does not account low priority table as part of their
// resource consumption.
static StageUseEstimate get_current_stage_use(const TablePlacement::Placed *pl,
                                              bool accumulate_low_pri = true) {
    StageUseEstimate    rv;
    if (pl) {
        int stage = pl->stage;
        gress_t gress = pl->table->gress;
        for (; pl && pl->stage == stage; pl = pl->prev) {
            if (!Device::threadsSharePipe(pl->table->gress, gress)) continue;
            if (!accumulate_low_pri && pl->table->get_placement_priority_int() < 0) continue;
            rv += pl->use; } }
    return rv;
}

// Retrieve the total resources for a given placement
static StageUseEstimate get_total_stage_use(const TablePlacement::Placed *pl) {
    StageUseEstimate    rv;
    for (; pl ; pl = pl->prev)
        rv += pl->use;
    return rv;
}

// Count the number of tables being placed
static int count(const TablePlacement::Placed *pl) {
    int rv = 0;
    while (pl) {
        pl = pl->prev;
        rv++; }
    return rv;
}

DecidePlacement::DecidePlacement(TablePlacement &s) : self(s) {}

/** encapsulates the data needed to backtrack and continue on an alternate placement path
 * a BacktrackPlacement object can be created from any not-yet-placed Placed object
 * and the worklist it was chosen from.  calling reset_place_table will reset and place
 * the Placed object, updating the worklist to continue from that point,
 */
class DecidePlacement::BacktrackPlacement {
    DecidePlacement &self;
    const Placed *pl;
    ordered_set<const GroupPlace*> work;
    bool is_best;  // Set to true if this placement was initially selected
    int init_cnt;  // Backtrack count value when saved
    ordered_map<cstring, bool> tablePlacementErrors;
    bool resource_mode;

 public:
    bool tried = false;  // Set to true if this placement was tried
    BacktrackPlacement(DecidePlacement &self, const Placed *p,
                       const ordered_set<const GroupPlace *> &w, bool ib) :
        self(self), pl(p), work(w), is_best(ib) {
        init_cnt = self.backtrack_count;
        tablePlacementErrors = self.self.summary.getPlacementError();
        resource_mode = self.resource_mode; }
    BacktrackPlacement &operator=(const BacktrackPlacement &a) {
        if (this == &a) return *this;
        BUG_CHECK(&self == &a.self, " inconsistent backtracking assignment");
        pl = a.pl;
        work = a.work;
        is_best = a.is_best;
        init_cnt = a.init_cnt;
        tablePlacementErrors = a.tablePlacementErrors;
        resource_mode = a.resource_mode;
        return *this; }
    const Placed *operator->() const { return pl; }
    const Placed *get_placed() const { return pl; }

    // Optionally increment or not the backtrack count. The backtrack count should not be
    // incremented if the backtracking mechanism is being used to evaluate multiple solutions.
    const Placed *reset_place_table(ordered_set<const GroupPlace *> &w, bool bt_inc = true) {
        LOG_FEATURE("stage_advance", 2, "backtracking to stage " << pl->stage);
        tried = true;
        if (bt_inc)
            ++self.backtrack_count;
        w = work;
        self.self.summary.setPlacementError(tablePlacementErrors);
        self.resource_mode = resource_mode;
        return self.place_table(w, pl); }
    int get_num_placement_errors() const { return tablePlacementErrors.size(); }

    bool is_best_placement() const { return is_best; }
    void reset_best_placement() { is_best = false; }
    bool is_same_backtrack_pass() const { return self.backtrack_count == init_cnt; }
    bool is_resource_mode() const { return resource_mode; }
};

/** Structure that saves various backtracking point for each table. The first index being the stage
 *  where this backtracking point is located and the second is the backtracking point itself. We
 *  use two maps to carry global and local backtracking point for each table. Global refer to the
 *  earliest time this table was placable per stage while local is rebuilt when a table is eligible
 *  and have a different backtrack count than previously found. E.g.:
 *
 *  Stage 0 : Eligible Tables = {T0, T1, T2, T3, T4}, Placed Tables = {T0, T3}
 *  Stage 1 : Eligible Tables = {T1, T2, T4, T5}, Placed Tables = {T1, T5}
 *  Stage 2 : Eligible Tables = {T2, T4, T6}, Placed Tables = {T4}
 *
 *  At this point the early(global) and last_pass(local) map will be the same for all tables
 *
 *  T0 -> Stage 0
 *  T1 -> Stage 0, 1
 *  T2 -> Stage 0, 1, 2
 *  T3 -> Stage 0
 *  T4 -> Stage 0, 1, 2
 *  T5 -> Stage 1
 *  T6 -> Stage 2
 *
 *  At this point if we found that placing table T6 would cause a fitting issue because the downward
 *  dependency chain will exceed the resource of the underlying chip, the backtracking mechanism
 *  will kick in and try to find an earlier placement for T6. Based on our current map, T6 don't
 *  have any previous stages where it was eligible. In this case, the backtracking mechanism will
 *  look at control and data dependency for this table and might find that T4 have a control
 *  dependency on T6 and if we move that table earlier we can probably fix the T6 dependency
 *  problem. By looking at T4 mapping we can see that this table was eligible at stage 0 and 1 as
 *  well. Stage 1 will probably be selected at backtracking point if the dependency chain was only
 *  missing 1 stage. The result might look like:
 *
 *  Stage 0 : Eligible Tables = {T0, T1, T2, T3, T4}, Placed Tables = {T0, T3}
 *  Stage 1 : Eligible Tables = {T1, T2, T4, T5, T6}, Placed Tables = {T4, T6}
 *  Stage 2 : Eligible Tables = {T1, T2, T5}, Placed Tables = {T1, T5}
 *
 *  After that backtracking point, the early(global) and last_pass(local) mapping will start to
 *  diverge. The backtracking mechanism will always try to find a local point before looking at
 *  global one to increase the solution instead of changing it entirely:
 *
 *         |            Local           |           Global          |
 *  T0 ->  | 0 (Same as original)       | 0                         |
 *  T1 ->  | 1, 2 (BT include T4)       | 0, 1, 2 (Stage 2 is new)  |
 *  T2 ->  | 1, 2 (BT include T4)       | 0, 1, 2                   |
 *  T3 ->  | 0 (Same as original)       | 0                         |
 *  T4 ->  | 1 (Tried flag set)         | 0, 1, 2                   |
 *  T5 ->  | 1, 2 (BT include T4)       | 1, 2 (Stage 2 is new)     |
 *  T6 ->  | 1 (BT include T4)          | 1, 2 (Stage 1 is new)     |
 */
struct DecidePlacement::save_placement_t {
    std::map<int, BacktrackPlacement>  early;  // global earliest time we've found where table
                                               // is placeable
    std::map<int, BacktrackPlacement>  last_pass;  // local earliest position. Local point are
                                                   // always updated when revisited with a new
                                                   // backtrack count
};

void DecidePlacement::savePlacement(const Placed *pl, const ordered_set<const GroupPlace *> &work,
                                    bool is_best) {
    if (saved_placements.count(pl->name)) {
        auto &info = saved_placements.at(pl->name);
        // Local backtrack point management
        if (!info.last_pass.begin()->second.is_same_backtrack_pass()) {
            info.last_pass.clear();
            info.last_pass.insert({pl->stage, BacktrackPlacement(*this, pl, work, is_best)});
        } else {
            if (info.last_pass.count(pl->stage)) {
                BacktrackPlacement &actual_pl = info.last_pass.at(pl->stage);
                if (actual_pl.tried || (actual_pl->need_more && !pl->need_more) ||
                    (actual_pl->entries < pl->entries && actual_pl->need_more == pl->need_more)) {
                    info.last_pass.erase(pl->stage);
                    info.last_pass.insert({pl->stage, BacktrackPlacement(*this, pl, work,
                                                                         is_best)});
                }
            } else {
                info.last_pass.insert({pl->stage, BacktrackPlacement(*this, pl, work, is_best)});
            }
        }
        // Global backtrack point management
        if (info.early.count(pl->stage)) {
            BacktrackPlacement &actual_pl = info.early.at(pl->stage);
            if (actual_pl.tried || (actual_pl->need_more && !pl->need_more)) {
                info.early.erase(pl->stage);
                info.early.insert({pl->stage, BacktrackPlacement(*this, pl, work, is_best)});
            } else if ((actual_pl->need_more == pl->need_more) &&
                       (actual_pl->entries < pl->entries || (actual_pl->entries == pl->entries &&
                        !actual_pl.is_same_backtrack_pass()))) {
                info.early.erase(pl->stage);
                info.early.insert({pl->stage, BacktrackPlacement(*this, pl, work, is_best)});
            } else {
                actual_pl.reset_best_placement();
            }
        } else {
            info.early.insert({pl->stage, BacktrackPlacement(*this, pl, work, is_best)});
        }
    } else {
        // Initial backtrack point
        std::map<int, BacktrackPlacement> early;
        early.insert({pl->stage, BacktrackPlacement(*this, pl, work, is_best)});
        std::map<int, BacktrackPlacement> last_pass;
        last_pass.insert({pl->stage, BacktrackPlacement(*this, pl, work, is_best)});
        saved_placements.emplace(pl->name, save_placement_t { early, last_pass });
    }
}

/** Figure out how many long branch tags will be needed across the last stage in `done` if
 * no more tables are placed in that stage.
 * For each group (tableSeq) in the worklist, it will need a long branch tag iff
 *  - it is control dependent on a table not in the last stage
 *  - it is not completely placed
 *  - none of its tables were placed in the last stage.  For this a split table is
 *    considered only as being in the first stage it has match entries in.
 * while calculating that, look for a good place to backtrack to in case we need to,
 * storing that in *bt
 */
int DecidePlacement::longBranchTagsNeeded(const Placed *done,
                                          const ordered_set<const GroupPlace *> &work,
                                          BacktrackPlacement **bt) {
    bitvec last_stage_tables;  // tables in the last stage of done
    int rv = 0;
    *bt = nullptr;
    for (auto *p = done; p && p->stage == done->stage; p = p->prev) {
        if (!p->use.format_type.matchEarlierStage())
            last_stage_tables[self.uid(p->table)] = 1;
        if (p->gw)
            last_stage_tables[self.uid(p->gw)] = 1; }
    for (auto *seq : work) {
        BUG_CHECK(done->placed.contains(seq->info.parents),
                  "parent(s) of seq on work list not placed?");
        if (last_stage_tables.contains(seq->info.parents)) continue;
        if (done->placed.contains(seq->info.immed_tables)) continue;
        if (last_stage_tables.intersects(seq->info.immed_tables)) continue;
        if (!*bt || (**bt)->stage != done->stage) {
            for (auto t : seq->info.immed_tables - done->placed) {
                auto *tbl = self.tblByUid.at(t)->table;
                if (!saved_placements.count(tbl->name)) continue;
                auto &info = saved_placements.at(tbl->name);
                if (info.last_pass.count(done->stage) && !info.last_pass.at(done->stage).tried) {
                    *bt = &info.last_pass.at(done->stage);
                    break; }
                if (info.early.count(done->stage) && !info.early.at(done->stage).tried) {
                    *bt = &info.early.at(done->stage);
                    break; } } }
        ++rv; }
    return rv;
}

class DecidePlacement::PlacementScore {
    DecidePlacement &self;
    // This backtrack placement point to the best table allocated in the next stage which also
    // carry all of the current stage placement.
    BacktrackPlacement *bt;
    // These backtrack point are all of the other non-best first table possible placements on the
    // next stage.
    ordered_map<const IR::MAU::Table *, BacktrackPlacement *> no_best;
    // Metric relative to resource being allocated for this stage for this allocation
    StageUseEstimate stage_use;

    // Carry the metric for a given downward dependency value
    struct stage_metric_t {
        // Number of tables in this downward dependency set
        int num_dep_chain = 0;
        // Number of incomplete tables in this downward dependency set
        int num_incomplete_dep_chain = 0;
        // Number of entries that refer to incomplete tables in this downward dependency set
        int entries_incomplete_dep_chain = 0;
        // List of completed table names for this set
        cstring tables_completed = ""_cs;
        // List of incomplete table names for this set
        cstring tables_incomplete = ""_cs;
    };
    // Map of downward dependency table set metrics
    std::map<int, stage_metric_t> stage_metric;

    // Look down to which downward dependency value to accept a solution.
    static constexpr int MaxDepLookup = 1;

    // Weight of each metrics used to compare and select the best allocation. Initially all of the
    // resources have the same weight but after the first table allocation pass, each of them are
    // re-computed to reflect the actual program needs. The weight is also updated during the
    // placement to increase or decrease each individual resource priority based on previous
    // placement. For exemple, if a given program uses 80% of the total SRAM, 50% of the TCAM and
    // 25% of the Logical Id and after some stages the previous stages was filled with something
    // like: 85% of total SRAM, 40% of the TCAM and 35% of the Logical Id then the weight will be
    // re-computed to:
    //     SRAM: 80% - 5% = 75% because we were doing a bit better than required so far
    //     TCAM: 50% + 10% = 60% because we were behind in this area
    //     LogId: 25% - 6.25% = 18.75% because the delta can't cross 1/4 of the original value
    int tcam_weight = 250;
    int sram_weight = 250;
    int logid_weight = 250;
    int map_weight = 250;

    void fill_score() {
        const Placed *pl = bt->get_placed();
        if (pl->prev) {
            pl = pl->prev;
            // Get the current stage uses without low priority table. This is to make sure that
            // if we have a solution with a large low priority table it would not be selected
            // vs another solution without low priority table but consuming a little less
            // resources.
            stage_use = get_current_stage_use(pl, false);
            int init_stage = pl->stage;

            StageUseEstimate total_res = self.self.summary.getAllStagesResources();
            // Make sure it is filled with valid information
            if (total_res.logical_ids) {
                int delta;
                StageUseEstimate current_res = get_total_stage_use(pl);

                // Using integer to carry floating point, 40.5% would be expressed as 405.
                int cur_tcam_usage = (current_res.tcams * 1000) /
                                     (StageUse::MAX_TCAMS * (init_stage + 1));
                int cur_sram_usage = (current_res.srams * 1000) /
                                     (StageUse::MAX_SRAMS * (init_stage + 1));
                int cur_logid_usage = (current_res.logical_ids * 1000) /
                                      (StageUse::MAX_LOGICAL_IDS * (init_stage + 1));
                int cur_map_usage = (current_res.maprams * 1000) /
                                    (StageUse::MAX_MAPRAMS * (init_stage + 1));

                int tot_tcam_usage = (total_res.tcams * 1000) /
                                     (StageUse::MAX_TCAMS * Device::numStages());
                int tot_sram_usage = (total_res.srams * 1000) /
                                     (StageUse::MAX_SRAMS * Device::numStages());
                int tot_logid_usage = (total_res.logical_ids * 1000) /
                                      (StageUse::MAX_LOGICAL_IDS * Device::numStages());
                int tot_map_usage = (total_res.maprams * 1000) /
                                    (StageUse::MAX_MAPRAMS * Device::numStages());

                LOG3("tot_tcam_usage:" << tot_tcam_usage << " tot_sram_usage:" << tot_sram_usage <<
                     " tot_logid_usage:" << tot_logid_usage << " tot_map_usage:" << tot_map_usage);
                LOG3("cur_tcam_usage:" << cur_tcam_usage << " cur_sram_usage:" << cur_sram_usage <<
                     " cur_logid_usage:" << cur_logid_usage << " cur_map_usage:" << cur_map_usage);

                // The delta can't exceed 1/4 of the original total usage.
                delta = tot_tcam_usage - cur_tcam_usage;
                if (delta && -delta > (tot_tcam_usage / 4)) delta = -(tot_tcam_usage / 4);
                tcam_weight = tot_tcam_usage + delta;
                LOG3("tcam_weight:" << tcam_weight << " delta:" << delta);

                delta = tot_sram_usage - cur_sram_usage;
                if (delta && -delta > (tot_sram_usage / 4)) delta = -(tot_sram_usage / 4);
                sram_weight = tot_sram_usage + delta;
                LOG3("sram_weight:" << sram_weight << " delta:" << delta);

                delta = tot_logid_usage - cur_logid_usage;
                if (delta && -delta > (tot_logid_usage / 4)) delta = -(tot_logid_usage / 4);
                logid_weight = tot_logid_usage + delta;
                LOG3("logid_weight:" << logid_weight << " delta:" << delta);

                delta = tot_map_usage - cur_map_usage;
                if (delta && -delta > (tot_map_usage / 4)) delta = -(tot_map_usage / 4);
                map_weight = tot_map_usage + delta;
                LOG3("map_weight:" << map_weight << " delta:" << delta);
            }

            while (pl) {
                if (pl->stage != init_stage)
                    break;
                // FIXME -- should skip tables in other gress if it has its own pipe?
                // or maybe always skip tables in other gress as they don't interact
                // control-dependency wise.

                // The "dep_stages_control_anti_split" variant was added to use previous table
                // stages usage for a more accurate compute.
                int dep_chain = self.get_control_anti_split_adj_score(pl);
                stage_metric_t metric = stage_metric[dep_chain];

                metric.num_dep_chain++;
                if (pl->need_more_match) {
                    metric.num_incomplete_dep_chain++;
                    metric.entries_incomplete_dep_chain += pl->entries;
                    metric.tables_incomplete += pl->name;
                    metric.tables_incomplete += " ";
                } else {
                    metric.tables_completed += pl->name;
                    metric.tables_completed += " ";
                }
                stage_metric[dep_chain] = metric;

                pl = pl->prev;
            }
        }
    }

 public:
    PlacementScore(DecidePlacement &self, BacktrackPlacement *bt) :
        self(self), bt(bt) { fill_score(); }
    int get_score() const {
        return (((tcam_weight * stage_use.tcams * 100) / StageUse::MAX_TCAMS) +
                ((sram_weight * stage_use.srams * 100) / StageUse::MAX_SRAMS) +
                ((logid_weight * stage_use.logical_ids * 100) / StageUse::MAX_LOGICAL_IDS) +
                ((map_weight * stage_use.maprams * 100) / StageUse::MAX_MAPRAMS)); }

    // Used to compare two placement score to find which one is the best
    bool operator>(const PlacementScore &other) const {
        // Only 1 level deep by default. Can be increased or decreased to increase or decrease
        // the dependency requirement over the resources metric.
        int dep_stage_wins_over_res = MaxDepLookup;
        auto lit = stage_metric.rbegin();
        auto rit = other.stage_metric.rbegin();
        while (dep_stage_wins_over_res) {
            if (lit == stage_metric.rend()) return false;
            if (rit == other.stage_metric.rend()) return true;

            if (lit->first > rit->first) return true;
            else if (lit->first < rit->first) return false;

            if (bt->get_num_placement_errors() > other.bt->get_num_placement_errors())
                return false;
            else if (bt->get_num_placement_errors() < other.bt->get_num_placement_errors())
                return true;

            const stage_metric_t &lmetric = lit->second;
            const stage_metric_t &rmetric = rit->second;

            if (lmetric.num_dep_chain > rmetric.num_dep_chain) return true;
            else if (lmetric.num_dep_chain < rmetric.num_dep_chain) return false;

            // Prefer complete table over incomplete placement to minimize logical id usage
            if (lmetric.num_incomplete_dep_chain > rmetric.num_incomplete_dep_chain)
                return false;
            else if (lmetric.num_incomplete_dep_chain < rmetric.num_incomplete_dep_chain)
                return true;

            // Prefer incomplete table with more entries on critical path
            if (lmetric.entries_incomplete_dep_chain > rmetric.entries_incomplete_dep_chain)
                return true;
            else if (lmetric.entries_incomplete_dep_chain < rmetric.entries_incomplete_dep_chain)
                return false;

            // Only look for next level if that one is adjacent to the prior one
            if (!stage_metric.count(lit->first - 1) && !other.stage_metric.count(rit->first - 1))
                break;

            ++lit;
            ++rit;
            dep_stage_wins_over_res--;
        }
        LOG3("score:" << get_score() << " other_score:" << other.get_score());

        if (get_score() > other.get_score())
            return true;
        else
            return false; }
    bool operator<(const PlacementScore &other) const { return *this > other; }
    BacktrackPlacement *get_backtrack() { return bt; }
    void add_no_best_bt(const IR::MAU::Table *tbl, BacktrackPlacement *bt) {
        no_best.insert({tbl, bt});
    }
    ordered_map<const IR::MAU::Table *, BacktrackPlacement *> &get_no_best_bt() { return no_best; }

    void printScore() const {
        LOG3("    num_log_id:" << stage_use.logical_ids);
        LOG3("    total_sram:" << stage_use.srams);
        LOG3("    total_tcam:" << stage_use.tcams);
        LOG3("    total_map:" << stage_use.maprams);
        LOG3("    total_eixbar:" << stage_use.exact_ixbar_bytes);
        LOG3("    total_tixbar:" << stage_use.ternary_ixbar_groups);
        for (auto it = stage_metric.rbegin(); it != stage_metric.rend(); ++it) {
            const stage_metric_t &metric = it->second;
            LOG3("    dep_chain:" << it->first);
            LOG3("        num:" << metric.num_dep_chain);
            LOG3("        incomplete_tbl:" << metric.num_incomplete_dep_chain);
            LOG3("        incomplete_entries:" << metric.entries_incomplete_dep_chain);
            LOG3("        tables_completed:" << metric.tables_completed);
            LOG3("        tables_incomplete:" << metric.tables_incomplete);
        }
    }
};

class DecidePlacement::ResourceBasedAlloc {
    // These references are used to rebuild the active placed and work object when backtracking
    DecidePlacement &self;
    ordered_set<const GroupPlace *> &active_work;
    ordered_set<const IR::MAU::Table *> &partly_placed;
    const Placed *&active_placed;

    // Carry the tables visited for this particular stage. The algo try to include all of the
    // visited table in at least one solution.
    ordered_set<const IR::MAU::Table *> visited;
    // These incomplete placement refer to branch point where multiple choices was available and
    // some of them included unvisited tables. Incomplete position will be revisited later using
    // backtracking to produce a complete solution.
    ordered_map<const IR::MAU::Table *, BacktrackPlacement *> incomplete;
    // Carry the various complete solutions that will be evaluated when all of the incomplete
    // placement will be finalized.
    ordered_set<PlacementScore *> complete;

 public:
    ResourceBasedAlloc(DecidePlacement &self, ordered_set<const GroupPlace *> &w,
                       ordered_set<const IR::MAU::Table *> &p, const Placed *&a) :
        self(self), active_work(w), partly_placed(p), active_placed(a) { }

    // Creating a backtracking position for this solution and return the placement score.
    PlacementScore *add_stage_complete(const Placed *pl,
                                       const ordered_set<const GroupPlace *> &work,
                                       const safe_vector<const Placed *> &trial,
                                       bool only_best) {
        BacktrackPlacement *bt = new BacktrackPlacement(self, pl, work, true);
        PlacementScore *pl_score = new PlacementScore(self, bt);
        if (!only_best) {
            for (auto t : trial) {
                if (t != pl && t->stage == pl->stage) {
                    BacktrackPlacement *not_best_bt = new BacktrackPlacement(self, t, work, false);
                    pl_score->add_no_best_bt(t->table, not_best_bt);
                }
            }
        }
        complete.insert(pl_score);
        return pl_score;
    }
    // Look for incomplete position to revisit. If at least one exist, backtrack to that position
    // and remove it from the incomplete set. This function returns true if other incomplete
    // placement exist and false otherwise.
    bool found_other_placement() {
        if (!incomplete.empty()) {
            auto bt_it = incomplete.begin();
            auto bt = bt_it->second;
            active_placed = bt->reset_place_table(active_work, false);
            self.recomputePartlyPlaced(active_placed, partly_placed);
            incomplete.erase(bt_it->first);
            return true;
        }
        return false;
    }
    // This should be called when all the incomplete solution was finalized to compare and select
    // the best solution out of the complete set. This function return true if the best solution
    // is the current one and false otherwise. The idea being that if the best solution is the
    // current one, we can continue the allocation without any backtracking.
    bool select_best_solution(const Placed *pl, PlacementScore *cur_score,
                              std::list<BacktrackPlacement *> &initial_stage_options) {
        // Compare the different solution and pick the best
        PlacementScore *best_score = cur_score;
        for (PlacementScore *pl_score : complete) {
            LOG3("Comparing placement score:");
            pl_score->printScore();
            if (*pl_score > *best_score) {
                LOG3("Updating best score with current based on metrics");
                best_score = pl_score;
            }
        }
        LOG3("Selected best placement with score:");
        best_score->printScore();

        initial_stage_options.clear();
        initial_stage_options.push_back(best_score->get_backtrack());
        for (auto bt : best_score->get_no_best_bt())
            initial_stage_options.push_back(bt.second);

        // Do not backtrack if the current solution is the best or if the solution goes beyond the
        // number of stages of the device and backtracking is still enabled. The idea for this
        // last use case is to let the backtracking mechanism find a better backtracking point to
        // relaxe the actual allocation constraint.
        if (best_score != cur_score &&
            (pl->stage < Device::numStages() ||
             (pl->stage >= Device::numStages() &&
              (self.backtrack_count > self.MaxBacktracksPerPipe)))) {
            auto bt = best_score->get_backtrack();
            active_placed = bt->reset_place_table(active_work, false);
            self.recomputePartlyPlaced(active_placed, partly_placed);
            incomplete = best_score->get_no_best_bt();
            complete.clear();
            visited.clear();
            return false;
        }
        complete.clear();
        visited.clear();
        return true;
    }
    // Eligible tables are considered as a backtrack point if not already visited. The best table
    // is considered as part of the next solution which is why incomplete solution based on this
    // node is removed.
    void add_placed_pos(const Placed *pl, const ordered_set<const GroupPlace *> &work,
                        bool is_best) {
        if (is_best) {
            if (incomplete.count(pl->table)) {
                BacktrackPlacement *bt = incomplete.at(pl->table);
                // Keep the table in the incomplete pool if the table is split and we know we can
                // fit more entries with a previous non best allocation
                if (!pl->need_more || bt->get_placed()->entries <= pl->entries)
                    incomplete.erase(pl->table);
            }
        } else if (!visited.count(pl->table)) {
            BacktrackPlacement *bt = new BacktrackPlacement(self, pl, work, false);
            incomplete.insert({pl->table, bt});
        }
        visited.insert(pl->table);
    }
    void clear_stage() {
        complete.clear();
        incomplete.clear();
        visited.clear();
    }
};

/** Class used to select the best solution between multiple complete attempts. The best solution is
 *  defined as the first one that meet the target stage requirement or the least number of stages in
 *  case none of them meet it. The typical sequence being evaluated by the table placement is:
 *
 *  1 - Table selected by dependency only
 *  2 - Table selected by dependency only with backtracking
 *  3 - Table selected by a combination of resources tracking + dependency
 *  4 - Table selected by a combination of resources tracking + dependency with backtracking
 *
 *  The table placement for each strategy will stop and be saved as an incomplete placement if at
 *  some point, the algorithm detect that downward dependency requirement are impossible to reach.
 *  When such incomplete placement is saved, the next strategy is selected. If none of these
 *  strategy is found to produce a placement that fit the target, all of the incomplete placement
 *  will be finalized and compared. The one that require the least stages will be selected as the
 *  final one. The first strategy are prioritize over the last if multiple strategy require the same
 *  number of stages.
 */
class DecidePlacement::FinalPlacement {
    // These references are used to rebuild the active placed and work object when backtracking
    DecidePlacement &self;
    ordered_set<const GroupPlace *> &active_work;
    ordered_set<const IR::MAU::Table *> &partly_placed;
    const Placed *&active_placed;

    ordered_set<BacktrackPlacement *> incomplete;
    ordered_set<BacktrackPlacement *> complete;

    // Simple hack to make sure the last strategy tried is evaluated with the least priority.
    BacktrackPlacement *first_complete = nullptr;

 public:
    FinalPlacement(DecidePlacement &self, ordered_set<const GroupPlace *> &w,
                   ordered_set<const IR::MAU::Table *> &p, const Placed *&a) :
        self(self), active_work(w), partly_placed(p), active_placed(a) { }

    void add_incomplete_placement(BacktrackPlacement * bp) {
        LOG3("Adding incomplete placement for resource mode:" << self.resource_mode <<
             " and backtracking attempt:" << self.backtrack_count);
        incomplete.insert(bp);
    }

    void add_complete_placement(BacktrackPlacement * bp) {
        LOG3("Adding complete placement for resource mode:" << self.resource_mode);
        // The first completed placement is in fact the last strategy tried. Just re-order them
        // properly for final evaluation.
        if (first_complete == nullptr)
            first_complete = bp;
        else
            complete.insert(bp);

        if ((bp->get_placed() == nullptr) || (bp->get_placed()->stage < Device::numStages())) {
            LOG3("Found a complete solution that fit the number of stages required");
            return;
        }
        if (!incomplete.empty()) {
            auto bt_it = incomplete.begin();
            auto bt = *bt_it;
            active_placed = bt->reset_place_table(active_work, false);
            self.recomputePartlyPlaced(active_placed, partly_placed);
            BUG_CHECK(self.resource_mode == bt->is_resource_mode(),
                      "Inconsistent resource mode flag");
            incomplete.erase(bt_it);
            LOG3("Finalizing incomplete placement for resource mode:" << self.resource_mode);
        }
    }

    void select_best_final_placement() {
        BUG_CHECK(first_complete != nullptr,
                  "Analyzing best final placement with no first complete solution");
        // Re-ordering hack
        complete.insert(first_complete);
        // Impossible default values
        int best_err = 1000;
        int best_num_stages = 1000;
        const Placed *best_pl = nullptr;
        for (const BacktrackPlacement *solution : complete) {
            const Placed *pl = solution->get_placed();
            int pl_stages = (pl ? pl->stage + 1 : 0);
            int placement_err = solution->get_num_placement_errors();
            LOG3("Evaluating complete solution with resource:" << solution->is_resource_mode());
            LOG3("Placement error(s):" << placement_err << " stages required:" << pl_stages);

            if (placement_err <= best_err && pl_stages < best_num_stages) {
                LOG3("Updating best final placement with this one");
                best_err = placement_err;
                best_num_stages = pl_stages;
                best_pl = pl;
            }
        }
        active_placed = best_pl;
        self.recomputePartlyPlaced(active_placed, partly_placed);
    }
};

namespace {
class StageSummary {
    std::unique_ptr<IXBar>      ixbar;
    std::unique_ptr<Memories>   mem;
 public:
    StageSummary(int stage, const TablePlacement::Placed *pl)
    : ixbar(IXBar::create()), mem(Memories::create()) {
        while (pl && pl->stage > stage) pl = pl->prev;
        while (pl && pl->stage == stage) {
            ixbar->update(pl->table, &pl->resources);
            mem->update(pl->resources.memuse);
            pl = pl->prev; } }
    friend std::ostream &operator<<(std::ostream &out, const StageSummary &sum) {
        return out << *sum.ixbar << Log::endl << *sum.mem; }
};
}  // end anonymous namespace

class DecidePlacement::Backfill {
    /** Used to track tables that could be backfilled into a stage if we run out of other
     *  things to put in that stage.  Whenever we choose to place table such that another
     * table is no longer (immediately) placeable due to next table limits (so only applies
     * to tofino1), we remember that (non-placed) table and if we later run out of things to
     * put into the current stage, we attempt to 'backfill' that remembered table -- place
     * it in the current stage *before* the table that made it non-placeable.
     *
     * Currently backfilling is limited to single tables that have no control dependent tables.
     * We could try backfilling multiple tables but that is less likely to be possible or
     * useful; it is more likely that a general backtracking scheme would be a better approach.
     */
    int                         stage = -1;
    struct table_t {
        const IR::MAU::Table    *table;
        cstring                 before;
    };
    std::vector<table_t>        avail;

 public:
    explicit Backfill(DecidePlacement &) {}
    explicit operator bool() const { return !avail.empty(); }
    std::vector<table_t>::iterator begin() { return avail.begin(); }
    std::vector<table_t>::iterator end() { return avail.end(); }
    void set_stage(int st) {
        if (stage != st) avail.clear();
        stage = st; }
    void clear() { avail.clear(); }
    void add(const Placed *tbl, const Placed *before) {
        set_stage(before->stage);
        avail.push_back({ tbl->table, before->name }); }
};

void DecidePlacement::GroupPlace::finish_if_placed(
    ordered_set<const GroupPlace*> &work, const Placed *pl
) const {
    if (pl->is_fully_placed(seq)) {
        LOG4("    Finished a sequence (" << seq->clone_id << ")");
        finish(work);
        for (auto p : parents)
            p->finish_if_placed(work, pl);
    } else {
        LOG4("    seq " << seq->clone_id << " not finished"); }
}

TablePlacement::GatewayMergeChoices
        TablePlacement::gateway_merge_choices(const IR::MAU::Table *table) {
    GatewayMergeChoices rv;
    // Abort and return empty if we're not a gateway
    if (!table->uses_gateway() || table->match_table) {
        LOG2(table->name << " is not a gateway! Aborting search for merge choices");
        return rv;
    }
    std::set<cstring>   gw_tags;
    for (auto &row : table->gateway_rows)
        gw_tags.insert(row.second);

    // Now, use the same criteria as gateway_merge to find viable tables
    for (auto it = table->next.rbegin(); it != table->next.rend(); it++) {
        if (seqInfo.at(it->second).refs.size() > 1) continue;
        bool multiple_tags = false;
        for (auto &el : table->next) {
            if (el.first != it->first && el.second == it->second) {
                multiple_tags = true;
                break; } }
        // FIXME -- if the sequence is used for more than one tag in the gateway, we can't
        // merge with any table in it as we only track one tag to replace.  Could change
        // GatewayMergeChoices and Placed to track a set of tags.
        if (multiple_tags) continue;

        int idx = -1;
        for (auto t : it->second->tables) {
            bool should_skip = false;
            ++idx;

            // table in more than one seq -- can't merge with gateway controling (only) one.
            if (tblInfo.at(t).refs.size() > 1) continue;

            if (it->second->deps[idx]) continue;

            // The TableSeqDeps does not currently capture dependencies like injected
            // control dependencies and metadata/dark initialization.  These have been folded
            // into the TableDependencyGraph and can be checked.
            // One could possibly fold this into TableSeqDeps, but only if initialization
            // happens in flow order, as the TableSeqDeps works with a LTBitMatrix
            for (auto t2 : it->second->tables) {
                if (deps.happens_logi_before(t2, t)) should_skip = true;
            }

            // If we have dependence ordering problems
            if (should_skip) continue;
            // If this table also uses gateways
            if (t->uses_gateway()) continue;
            // Always Run Instructions are not logical tables
            if (t->is_always_run_action()) continue;
            // Currently would potentially require multiple gateways if split into
            // multiple tables.  Not supported yet during allocation
            if (t->for_dleft()) continue;
            // Liveness problems
            if (!phv.darkLivenessOkay(table, t)) {
                LOG2("\tCannot merge " << table->name << " with " << t->name << " because of "
                     "liveness check");
                continue;
            }
            if (t->getAnnotation("separate_gateway"_cs)) {
                LOG2("\tCannot merge " << table->name << " with " << t->name << " because of "
                     "separate_gateway annotation");
                continue;
            }

            // Check if we have already seen this table
            if (rv.count(t))
                continue;
            rv[t] = it->first;
        }
    }
    return rv;
}

void TablePlacement::Placed::gateway_merge(const IR::MAU::Table *match, cstring result_tag) {
    // Check that the table we're attempting to merge with is a gateway
    BUG_CHECK((table->uses_gateway() || !table->match_table),
              "Gateway merge called on a non-gateway!");
    BUG_CHECK(!match->uses_gateway(), "Merging a non-match table into a gateway!");
    // Perform the merge
    name = match->name;
    gw = table;
    table = match;
    gw_result_tag = result_tag;
}

/** count the numbner of distinct stateful actions invoked on a stateful alu in this table
 * if there is more than one, then which salu action to run will need to be selected in the
 * meter_type part of the meter address bus, and can't be set from the default
 * FIXME -- is this correct?  It seems to count all the calls, regardless of which instruction
 * they invoke -- multiple table actions that all call the same salu action should jsut be
 * counted as one sful action.
 */
static int count_sful_actions(const IR::MAU::Table *tbl) {
    int rv = 0;
    for (auto act : Values(tbl->actions))
        if (!act->stateful_calls.empty())
            ++rv;
    return rv;
}

/**
 * @defgroup mau_resource_allocation MAU/PPU resource allocation
 * \brief Content related to resource allocation inside MAU/PPU.
 *
 * Methods for allocating resources in a single stage to meet the choices in Placed objects.
 * These methods all look at the placements for the latest stage on the front of the list and
 * try to fit everything in one stage.  They return true if they succeed and update the
 * 'resources' TableResourceAlloc object(s) to match.  Those that need to reallocate things
 * for the entire stage (not just the last table placed) take a table_resource_t containing
 * the TableResourceAlloc objects for all the trailing tables in the stage (as the 'prev'
 * pointers are const and cannot be updated directly.
 *
 * @{
 */

/* filter out options that do not meet the given layout flags
 */
void TablePlacement::filter_layout_options(Placed *pl) {
    safe_vector<LayoutOption>   ok;
    for (auto &lo : pl->use.layout_options) {
        if ((pl->stage_flags & StageFlag::Immediate) && lo.layout.action_data_bytes_in_table)
            continue;
        if ((pl->stage_flags & StageFlag::NoImmediate) && lo.layout.immediate_bits)
            continue;
        ok.emplace_back(std::move(lo)); }
    if (ok.empty()) {
        warning("No layouts for table %s match @stage %d flags (flags will be ignored)",
                pl->table, pl->stage);
        return; }
    pl->use.layout_options = ok;
}

bool TablePlacement::disable_split_layout(const IR::MAU::Table *tbl) {
    if (BackendOptions().disable_split_attached || limit_tmp_creation)
        return true;

    if (count_sful_actions(tbl) > 1)
        return true;

    for (auto *ba : tbl->attached) {
        // Stateful actions that don't require any indexes and are not supported by the actual
        // split attach framework.
        if (ba->use == IR::MAU::StatefulUse::FAST_CLEAR ||
            ba->use == IR::MAU::StatefulUse::STACK_PUSH ||
            ba->use == IR::MAU::StatefulUse::STACK_POP ||
            ba->use == IR::MAU::StatefulUse::FIFO_PUSH ||
            ba->use == IR::MAU::StatefulUse::FIFO_POP)
            return true;
    }
    return false;
}

/**
 * The estimates for potential layout options are determined before all information is possibly
 * known:
 *   1. What the actual input xbar layout of the table was, and whether this required more
 *      ixbar groups that estimated, either due to PHV constraints, or other resources
 *      on the input xbar
 *   2. Whether the table is tied to an gateway, and may need an extra bit for actions
 *   3. How many bits can be ghosted off.
 *
 * Thus, some layouts may not actually be possible that were precalculated.  This will adjust
 * potential layouts if the allocation can not fit within the pack format.
 *
 */
bool TablePlacement::pick_layout_option(Placed *next, std::vector<Placed *> allocated_layout) {
    LOG3("Picking layout option for table : " << next->name << " with requested entries : " <<
         next->entries);
    bool table_format = true;
    int req_entries = next->entries;

    if (!next->use.format_type.valid()) {
        bool disable_split = disable_split_layout(next->table);
        next->use = StageUseEstimate(next->table, next->entries, next->attached_entries, &lc,
                                     next->stage_split > 0, next->gw != nullptr, disable_split,
                                     phv);
        if (next->stage_flags) filter_layout_options(next); }

    LOG5("Layout options: " << next->use.layout_options);
    do {
        LOG5("preferred " << Log::indent << next->use.preferred() << Log::unindent);
        LOG5("Trying table format : " << std::boolalpha << table_format);
        bool ixbar_fit = try_alloc_ixbar(next, allocated_layout);
        if (!ixbar_fit) {
            next->stage_advance_log = "ran out of ixbar"_cs;
            return false;
        } else if (!next->use.format_type.matchThisStage()) {
            // if post-split, there's no match in this stage (just a gateway running the
            // attached table(s), so no need for match formatting
            table_format = true;
        } else if (!next->table->conditional_gateway_only() &&
                   !next->table->is_always_run_action()) {
            table_format = try_alloc_format(next, next->gw);
        }

        if (!table_format) {
            int in_out_entries = req_entries;
            bool adjust_possible = next->use.adjust_choices(next->table, in_out_entries,
                                                            next->attached_entries);
            if (!adjust_possible) {
                next->stage_advance_log = "adjust_choices failed"_cs;
                return false; }
        }
    } while (!table_format);
    LOG2("picked layout for " << next->name << " " << next->use.format_type << " " <<
         next->use.preferred());
    if (auto *lo = next->use.preferred()) {
        if (auto entries = lo->entries) {
            /* There's some confusion here as to exactly what 'entries == 0' means
             * The TableLayout code use that for "no-match" tables that have no match
             * entries, while the table placement code seems to use "entries == 1" for that.
             * May be a holdover from before FormatType_t was added, so could be cleaned up?
             * For now we avoid changing table placement's entries to 0, but otherwise we
             * update it to match what the chosen layout specifies */
            next->entries = entries; } }
    return true;
}

/* Estimate the layout options for all tables that are included in
 * vector tables_to_allocate, considering the layout options already
 * estimated for tables included in vector tables_placed as all these
 * tables are in the same stage.
 */
bool TablePlacement::try_pick_layout(const gress_t &gress,
                                     std::vector<Placed *> tables_to_allocate,
                                     std::vector<Placed *> tables_placed) {
    LOG3("Trying to allocate layout for " << tables_to_allocate);

    std::vector<Placed *> layout_allocated;
    for (auto *p : boost::adaptors::reverse(tables_placed)) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        layout_allocated.insert(layout_allocated.begin(), p);
    }

    for (auto *p : boost::adaptors::reverse(tables_to_allocate)) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        p->resources.clear_ixbar();
        if (!pick_layout_option(p, layout_allocated)) {
            // Invalidating intermediate table that fail layout option
            if (p != tables_to_allocate.front())
                p->use.format_type.invalidate();
            return false;
        }
        layout_allocated.insert(layout_allocated.begin(), p);
    }

    std::unique_ptr<IXBar> verify_ixbar(IXBar::create());
    for (auto *p : tables_placed) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        verify_ixbar->update(p->table, &p->resources);
    }
    for (auto *p : tables_to_allocate) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        verify_ixbar->update(p->table, &p->resources);
    }
    verify_ixbar->verify_hash_matrix();
    LOG7(IndentCtl::indent << IndentCtl::indent);
    LOG7(*verify_ixbar << IndentCtl::unindent << IndentCtl::unindent);
    return true;
}

bool TablePlacement::shrink_attached_tbl(Placed *next, bool first_time, bool &done_shrink) {
    for (auto *ba : next->table->attached) {
        auto *att = ba->attached;
        if (att->direct || next->attached_entries.at(att).entries == 0) continue;
        if (first_time && !can_split(next->table, att)) {
            if (!can_duplicate(att)) return false;
            continue; }
        if (first_time && attached_to.at(att).size() > 1 &&
            !next->attached_entries.at(att).need_more) {
            // a shared attached table may be shared by tables in whole_stage,
            // so they need recomputing layout, etc as well.
            BUG_CHECK(next->complete_shared > 0, "inconsistent shared table");
            next->complete_shared--; }
        if (first_time && next->entries > 0) {
            LOG3("  - splitting " << att->name << " to later stage(s)");
            next->attached_entries.at(att).entries = 0;
            next->attached_entries.at(att).need_more = true;
            next->use.format_type.invalidate();
            done_shrink = true;
        } else if (next->entries == 0) {
            // FIXME -- need a better way of reducing the size to what will fit in the stage
            // Also need to fix the memories.cpp code to adjust the numbers to match how
            // many entries are actually allocated if it is not an extact match up.
            // Also need some way of telling ixbar allocation that more entries will be
            // needed in later stages, so it can chain_vpn properly.
            int delta = 1 << std::max(10, ceil_log2(next->attached_entries.at(att).entries) - 4);
            if (delta <= next->attached_entries.at(att).entries) {
                next->attached_entries.at(att).entries -= delta;
                next->use.format_type.invalidate();
                if (!next->attached_entries.at(att).need_more) {
                    next->use.format_type.invalidate();
                    next->attached_entries.at(att).need_more = true; }
                LOG3("  - reducing size of " << att->name << " by " << delta << " to " <<
                     next->attached_entries.at(att).entries);
                done_shrink = true; }
        }
    }
    return true;
}

bool TablePlacement::shrink_estimate(Placed *next, int &srams_left, int &tcams_left,
                                     int min_entries) {
    if (next->table->is_a_gateway_table_only() || next->table->match_key.empty())
        return false;

    // No shrinking estimates for alt table placement round. This will happen
    // mostly because the phv allocation does not allow fitting the same no. of
    // entries as previous round. For now, we exit and continue with the default
    // placement round.
    if (summary.is_table_replay())
        return false;

    LOG2("Shrinking estimate on table " << next->name << " for min entries: " << min_entries);
    bool done_shrink = false;
    if (!shrink_attached_tbl(next, true, done_shrink))
        return false;

    if (done_shrink)
        return true;

    auto t = next->table;
    if (t->for_dleft()) {
        error("Table %1%: cannot split dleft hash tables", t);
        return false;
    }

    if (t->layout.atcam) {
        next->use.calculate_for_leftover_atcams(t, srams_left, next->entries,
                                                next->attached_entries);
    } else if (!t->layout.ternary) {
        if (!next->use.calculate_for_leftover_srams(t, srams_left, next->entries,
                                                    next->attached_entries)) {
            next->stage_advance_log = "ran out of srams"_cs;
            return false;
        }
    } else {
        next->use.calculate_for_leftover_tcams(t, tcams_left, srams_left, next->entries,
                                               next->attached_entries); }

    if (next->entries < min_entries) {
        LOG5("Couldn't place minimum entries within table " << t->name);
        if (t->layout.ternary)
            next->stage_advance_log = "ran out of tcams"_cs;
        else
            next->stage_advance_log = "ran out of srams"_cs;
        return false;
    }

    if (min_entries > 0)
        next->use.remove_invalid_option();

    LOG3("  - reducing to " << next->entries << " of " << t->name << " in stage " << next->stage);
    for (auto *ba : next->table->attached)
        if (ba->attached->direct)
            next->attached_entries.at(ba->attached).entries = next->entries;

    return true;
}

bool TablePlacement::shrink_preferred_lo(Placed *next) {
    const IR::MAU::Table* t = next->table;
    LOG3("Shrinking table resource of the preferred layout");
    bool done_shrink = false;
    if (!shrink_attached_tbl(next, false, done_shrink))
        return false;

    if (done_shrink)
        return true;

    if (t->layout.atcam) {
        next->use.shrink_preferred_atcams_lo(t, next->entries, next->attached_entries);
    } else if (!t->layout.ternary) {
        next->use.shrink_preferred_srams_lo(t, next->entries, next->attached_entries);
    } else {
        next->use.shrink_preferred_tcams_lo(t, next->entries, next->attached_entries);
    }

    if (next->use.layout_options.size() == 0) {
        LOG5("Couldn't place minimum entries within table " << t->name);
        if (t->layout.ternary)
            next->stage_advance_log = "ran out of tcams"_cs;
        else
            next->stage_advance_log = "ran out of srams"_cs;
        return false;
    }

    LOG3("  - reducing to " << next->entries << " of " << t->name << " in stage " << next->stage);
    for (auto *ba : next->table->attached)
        if (ba->attached->direct)
            next->attached_entries.at(ba->attached).entries = next->entries;

    return true;
}

struct TablePlacement::RewriteForSplitAttached : public Transform {
    TablePlacement &self;
    const Placed *pl;
    RewriteForSplitAttached(TablePlacement &self, const Placed *p)
        : self(self), pl(p) {}
    const IR::MAU::Table *preorder(IR::MAU::Table *tbl) {
        self.setup_detached_gateway(tbl, pl);
        // don't visit most of the children
        for (auto it = tbl->actions.begin(); it != tbl->actions.end();)  {
            if ((it->second = self.att_info.get_split_action(it->second, tbl, pl->use.format_type)))
                ++it;
            else
                it = tbl->actions.erase(it); }
        visit(tbl->attached, "attached");
        prune();
        return tbl; }
    const IR::MAU::BackendAttached *preorder(IR::MAU::BackendAttached *ba) {
        auto *tbl = findContext<IR::MAU::Table>();
        for (auto act : Values(tbl->actions)) {
            if (auto *sc = act->stateful_call(ba->attached->name)) {
                if (sc->index) {
                    if (auto *hd = sc->index->to<IR::MAU::HashDist>()) {
                        ba->hash_dist = hd;
                        break;
                    }
                }
            }
        }
        if (ba->attached->is<IR::MAU::Counter>() || ba->attached->is<IR::MAU::Meter>() ||
            ba->attached->is<IR::MAU::StatefulAlu>()) {
            ba->pfe_location = IR::MAU::PfeLocation::DEFAULT;
            if (!ba->attached->is<IR::MAU::Counter>())
                ba->type_location = IR::MAU::TypeLocation::DEFAULT;
        }
        prune();
        return ba; }
};

bool TablePlacement::try_alloc_ixbar(Placed *next, std::vector<Placed *> allocated_layout) {
    Log::TempIndent indent;
    LOG5("Trying to allocate ixbar for " << next->name << indent);
    next->resources.clear_ixbar();
    std::unique_ptr<IXBar> current_ixbar(IXBar::create());
    int tables_already_in_stage = 0;
    for (auto *p : boost::adaptors::reverse(allocated_layout)) {
        // TODO: SCM or ternary is shared across both gress
        if (!Device::threadsSharePipe(p->table->gress, next->table->gress)) continue;
        current_ixbar->update(p->table, &p->resources);
        tables_already_in_stage++;
    }
    current_ixbar->add_collisions();

    const ActionData::Format::Use *action_format = next->use.preferred_action_format();
    auto *table = next->table;
    if (next->entries == 0 && !table->conditional_gateway_only() &&
        !table->is_always_run_action()) {
        // detached attached table -- need to rewrite it as a hash_action gateway that
        // tests the enable bit it drives the attached table with the saved index
        // FIXME -- should we memoize this rather than recreating it each time?
        BUG_CHECK(!next->attached_entries.empty(),  "detaching attached from %s, no "
                  "attached entries?", next->name);
        BUG_CHECK(!next->gw, "Have a gateway merged with a detached attached table?");
        table = table->apply(RewriteForSplitAttached(*this, next));
    }

    if (!current_ixbar->allocTable(table, next->gw, phv, next->resources, next->use.preferred(),
                                   action_format, next->attached_entries)) {
        next->resources.clear_ixbar();
        error_message = "The table " + next->table->name + " could not fit within the "
                        "input crossbar";
        if (tables_already_in_stage)
            error_message += " with " + std::to_string(tables_already_in_stage) + " other tables";
        else
            error_message += " by itself";
        if (current_ixbar->failure_reason)
            error_message += ": " + current_ixbar->failure_reason;
        LOG3("    " << error_message);
        return false;
    }

    LOG5("Allocating ixbar successful");
    return true;
}

bool TablePlacement::try_alloc_mem(Placed *next, std::vector<Placed *> whole_stage) {
    Log::TempIndent indent;
    LOG5("Trying to allocate mem for " << next->name << indent);
    std::unique_ptr<Memories> current_mem(Memories::create());
    // This is to guarantee for Tofino to have at least a table per gress within a stage, as
    // a path is required from the parser
    std::array<bool, 2> gress_in_stage = { { false, false} };
    bool shrink_lt = false;
    if (Device::currentDevice() == Device::TOFINO) {
        if (next->stage == 0) {
            for (auto *p = next->prev; p && p->stage == next->stage; p = p->prev) {
                gress_in_stage[p->table->gress] = true;
            }
            gress_in_stage[next->table->gress] = true;
            for (int gress_i = 0; gress_i < 2; gress_i++) {
                if (table_in_gress[gress_i] && !gress_in_stage[gress_i])
                    shrink_lt = true;
            }
        }
    }

    if (shrink_lt)
        current_mem->shrink_allowed_lts();


    const IR::MAU::Table *table_to_add = nullptr;
    for (auto *p : whole_stage) {
        if (!Device::threadsSharePipe(p->table->gress, next->table->gress)) continue;
        table_to_add = p->table;
        if (!p->use.format_type.matchThisStage())
            table_to_add = table_to_add->apply(RewriteForSplitAttached(*this, p));
        BUG_CHECK(p != next && p->stage == next->stage, "invalid whole_stage");
        // Always Run Tables cannot be counted in the logical table check
        current_mem->add_table(table_to_add, p->gw, &p->resources, p->use.preferred(),
                               p->use.preferred_action_format(), p->use.format_type,
                               p->entries, p->stage_split, p->attached_entries);
        p->resources.memuse.clear(); }
    table_to_add = next->table;
    if (!next->use.format_type.matchThisStage())
        table_to_add = table_to_add->apply(RewriteForSplitAttached(*this, next));
    current_mem->add_table(table_to_add, next->gw, &next->resources, next->use.preferred(),
                           next->use.preferred_action_format(), next->use.format_type,
                           next->entries, next->stage_split, next->attached_entries);
    next->resources.memuse.clear();

    if (!current_mem->allocate_all()) {
        error_message = next->table->toString() + " could not fit in stage " +
                        std::to_string(next->stage) + " with " + std::to_string(next->entries)
                        + " entries";
        const char *sep = " along with ";
        for (auto &ae : next->attached_entries) {
            // All selector tables need a stateful table that can be used by the driver to set and
            // clear entries in the table. This should be abstracted from the customer when
            // reporting an error.
            if (auto *salu = ae.first->to<IR::MAU::StatefulAlu>())
                if (salu->synthetic_for_selector)
                    continue;

            if (ae.second.entries > 0) {
                error_message += sep + std::to_string(ae.second.entries) + " entries of " +
                                 ae.first->toString();
                sep = " and "; } }
        LOG3("    " << error_message);
        LOG3("    " << current_mem->last_failure());
        next->stage_advance_log = "ran out of memories: " + current_mem->last_failure();
        LOG3("Memuse for failed memory placement: ");
        LOG3(*current_mem);
        next->resources.memuse.clear();
        for (auto *p : whole_stage)
            p->resources.memuse.clear();
        return false;
    }

    std::unique_ptr<Memories> verify_mem(Memories::create());
    if (shrink_lt)
        verify_mem->shrink_allowed_lts();
    for (auto *p : whole_stage)
        verify_mem->update(p->resources.memuse);
    verify_mem->update(next->resources.memuse);
    LOG7(IndentCtl::indent << IndentCtl::indent);
    LOG7(*current_mem << IndentCtl::unindent << IndentCtl::unindent);
    LOG5("\t Allocating mem successful");
    return true;
}

bool TablePlacement::try_alloc_format(Placed *next, bool gw_linked) {
    LOG6("try_alloc_format(" << next->name << "): [" << next->use.preferred_index << "] " <<
         Log::indent << *next->use.preferred() << Log::unindent);
    const bitvec immediate_mask = next->use.preferred_action_format()->immediate_mask;
    next->resources.table_format.clear();
    gw_linked |= next->use.preferred()->layout.gateway_match;
    // If the placed table has been split some of the attached tables might
    // have been moved to its part (meter/counters/...)
    // Remove them, so that pack fields for them dont clutter the table
    // that no longer has those resources
    auto *tbl = next->table->clone();
    erase_if(tbl->attached, [next](const IR::MAU::BackendAttached *ba) {
        return !next->attached_entries.count(ba->attached) ||
               next->attached_entries.at(ba->attached).entries == 0; });

    auto* current_format = TableFormat::create(*next->use.preferred(),
            next->resources.match_ixbar.get(), next->resources.proxy_hash_ixbar.get(),
            tbl, immediate_mask, gw_linked, lc.fpc, phv);

    if (!current_format->find_format(&next->resources.table_format)) {
        next->resources.table_format.clear();
        error_message = "The selected pack format for table " + next->table->name + " could "
                        "not fit given the input xbar allocation";
        LOG3("    " << error_message);
        return false;
    }
    return true;
}

bool TablePlacement::try_alloc_adb(const gress_t &gress,
                                   std::vector<Placed *> tables_to_allocate,
                                   std::vector<Placed *> tables_placed) {
    LOG3("Trying to allocate adb for " << tables_to_allocate);

    std::vector<Placed *> adb_allocated;
    for (auto *p : boost::adaptors::reverse(tables_placed)) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        if (p->table->conditional_gateway_only()) continue;
        adb_allocated.insert(adb_allocated.begin(), p);
    }

    std::unique_ptr<ActionDataBus> current_adb(ActionDataBus::create());

    for (auto *p : boost::adaptors::reverse(tables_to_allocate)) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        if (p->table->conditional_gateway_only()) continue;
        BUG_CHECK(p->use.preferred_action_format() != nullptr,
                  "A non gateway table has a null action data format allocation");

        current_adb->clear();
        for (auto *q : boost::adaptors::reverse(adb_allocated))
            current_adb->update(q->name, &q->resources, q->table);

        p->resources.action_data_xbar.reset();
        p->resources.meter_xbar.reset();
        /*
         * allocate action data on adb
         */
        if (!current_adb->alloc_action_data_bus(p->table,
                p->use.preferred_action_format(), p->resources)) {
            error_message = "The table " + p->table->name + " could not fit in within "
                            "the action data bus";
            LOG3("    " << error_message);
            p->resources.action_data_xbar.reset();
            p->stage_advance_log = "ran out of action data bus space"_cs;
            return false;
        }

        /*
         * allocate meter output on adb
         */
        if (!current_adb->alloc_action_data_bus(p->table,
                p->use.preferred_meter_format(), p->resources)) {
            error_message = "The table " + p->table->name + " could not fit its meter "
                            " output in within the action data bus";
            LOG3(error_message);
            p->resources.meter_xbar.reset();
            p->stage_advance_log = "ran out of action data bus space for meter output"_cs;
            return false;
        }

        adb_allocated.insert(adb_allocated.begin(), p);
    }

    std::unique_ptr<ActionDataBus> verify_adb(ActionDataBus::create());
    for (auto *p : tables_placed) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        if (p->table->conditional_gateway_only()) continue;
        verify_adb->update(p->name, &p->resources, p->table);
    }
    for (auto *p : tables_to_allocate) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        if (p->table->conditional_gateway_only()) continue;
        verify_adb->update(p->name, &p->resources, p->table);
    }
    return true;
}

bool TablePlacement::try_alloc_imem(const gress_t &gress, std::vector<Placed *> tables_to_allocate,
                                    std::vector<Placed *> tables_placed) {
    LOG3("Trying to allocate imem for " << tables_to_allocate);

    std::unique_ptr<InstructionMemory> imem(InstructionMemory::create());

    for (auto *p : boost::adaptors::reverse(tables_placed)) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        if (p->table->conditional_gateway_only()) continue;
        imem->update(p->name, &p->resources, p->table);
    }

    for (auto *p : boost::adaptors::reverse(tables_to_allocate)) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        if (p->table->conditional_gateway_only()) continue;
        p->resources.instr_mem.clear();
        bool gw_linked = p->gw != nullptr;
        gw_linked |= p->use.preferred()->layout.gateway_match;
        if (!imem->allocate_imem(p->table, p->resources.instr_mem, phv, gw_linked,
                                 p->use.format_type, att_info)) {
            error_message = "The table " + p->table->name + " could not fit within the "
                            "instruction memory";
            LOG3("    " << error_message);
            p->resources.instr_mem.clear();
            p->stage_advance_log = "ran out of imem"_cs;
            return false; }
    }

    std::unique_ptr<InstructionMemory> verify_imem(InstructionMemory::create());
    for (auto p : tables_placed) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        verify_imem->update(p->name, &p->resources, p->table);
    }
    for (auto p : tables_to_allocate) {
        if (!Device::threadsSharePipe(p->table->gress, gress)) continue;
        verify_imem->update(p->name, &p->resources, p->table);
    }
    return true;
}

TableSummary::PlacementResult TablePlacement::try_alloc_all(Placed *next,
    std::vector<Placed *> whole_stage, const char *what, bool no_memory) {
    LOG3("Try_alloc_all for " << std::string(what));
    bool done_next = false;
    bool add_to_tables_placed = true;
    std::vector<Placed *> tables_to_allocate;
    std::vector<Placed *> tables_placed;

    // Insert each Placed object, i.e. 'next' and the ones included
    // in whole_stage, in one of the following two vectors:
    //
    //    - tables_placed if the object's resources have already
    //      been calculated and are still valid;
    //
    //    - tables_to_allocate if the object's resources need to
    //      be calculated or re-calculated.  Object 'next' always
    //      goes in tables_to_allocate.
    //
    // Place these objects so the resources are allocated in the order
    // tables appear in the pipeline.  This is the most efficient way
    // to allocate resources, since it ensures that the resources are
    // packed as much as possible.  Some profiles in the current CI
    // fail to compile when the table order is not respected.
    //
    // Finally, since in-order allocation is performed, when 'next'
    // is inserted in tables_to_allocate before any tables in
    // whole_stage, the tables that follow in whole_stage also
    // need to be recalculated.  This happens when a table is
    // backfilled.
    //
    for (auto *p : boost::adaptors::reverse(whole_stage)) {
        if (p->prev == next) {
            done_next = true;
            add_to_tables_placed = false;
            tables_to_allocate.insert(tables_to_allocate.begin(), next);
        }
        if (!p->use.format_type.valid())
            add_to_tables_placed = false;
        if (add_to_tables_placed)
            tables_placed.insert(tables_placed.begin(), p);
        else
            tables_to_allocate.insert(tables_to_allocate.begin(), p);
    }
    if (!done_next) {
        tables_to_allocate.insert(tables_to_allocate.begin(), next);
    }

    if (!try_pick_layout(next->table->gress, tables_to_allocate, tables_placed)) {
        LOG3("    " << what << " ixbar allocation did not fit");
        return TableSummary::FAIL_ON_IXBAR; }

    if (!try_alloc_adb(next->table->gress, tables_to_allocate, tables_placed)) {
        LOG3("    " << what << " of action data bus did not fit");
        return TableSummary::FAIL_ON_ADB; }

    if (!try_alloc_imem(next->table->gress, tables_to_allocate, tables_placed)) {
        LOG3("    " << what << " of instruction memory did not fit");
        return TableSummary::FAIL; }

    if (no_memory) return TableSummary::SUCC;
#if 0
    // SRAMs/MAPRAMs table consumption can be wrong if table share resources. It is probably
    // better to not stop here but let try_alloc_mem find a solution if one exist. The actual code
    // don't properly support mmultiple match table that share the same attach table. In this case
    // the SRAMs consumed by the shared table will be multiplied by the number of match table
    // that are connected to this latter. Because of this issue, we might think the SRAM array is
    // full while it is not and not attempt to insert this eligible table. Fixing this issue
    // would help our compilation time by not trying impossible memory allocation and simply
    // stop here.
    if (auto ran_out = get_current_stage_use(next).ran_out()) {
        if (error_message == "") {
            error_message = next->table->toString() + " could not fit in stage " +
                            std::to_string(next->stage) + " with " + std::to_string(next->entries)
                            + " entries";
            const char *sep = " along with ";
            for (auto &ae : next->attached_entries) {
                if (ae.second.entries > 0) {
                    error_message += sep + std::to_string(ae.second.entries) + " entries of " +
                                     ae.first->toString();
                    sep = " and "; } }
            error_message += ", ran out of " + ran_out; }
        LOG3("    " << what << " of memory allocation ran out of " << ran_out);
        next->stage_advance_log = "ran out of " + ran_out;
        return false;
    }
#endif
    if (!try_alloc_mem(next, whole_stage)) {
        LOG3("    " << what << " of memory allocation did not fit");
        return TableSummary::FAIL_ON_MEM; }
    return TableSummary::SUCC;
}

/** @} */  // end of alloc

/// Check an indirect attached table to see if it can be duplicated across stages, or if there
/// must be only a single copy of (each element of) the table.  This does not consider
/// whether it is a good idea to duplicate the table (not a good idea for large tables and
/// pointless if it won't fit in a single stage anyways).
bool TablePlacement::can_duplicate(const IR::MAU::AttachedMemory *att) {
    BUG_CHECK(!att->direct, "Not an indirect attached table");
    if (att->is<IR::MAU::Counter>() || att->is<IR::MAU::ActionData>() ||
        att->is<IR::MAU::Selector>())
        return true;
    if (auto *salu = att->to<IR::MAU::StatefulAlu>())
        return salu->synthetic_for_selector;
    return false;
}

/// Check if an indirect attached table can be split acorss stages
bool TablePlacement::can_split(const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *att) {
    BUG_CHECK(!att->direct, "Not an indirect attached table");
    if (Device::currentDevice() == Device::TOFINO) {
        // Tofino not supported yet -- need vpn check in gateway
        return false; }
    if (disable_split_layout(tbl))
        return false;
    if (att->is<IR::MAU::Selector>())
        return false;
    if (auto *salu = att->to<IR::MAU::StatefulAlu>()) {
        // FIXME is this synthetic_for_selector test still needed, or is it subsumed by the
        // selector test below.  If it is not still needed, could the flag go away completely?
        if (salu->synthetic_for_selector) return false;
        if (salu->selector) {
            // can't split the SALU if it is attached to a selector attached to the same table
            for (auto *ba : tbl->attached)
                if (ba->attached == salu->selector)
                    return false; }
        return true; }
    // non-stateful need vpn checks in gateway (as on Tofino1), as the JBay vpn offset/range
    // check only works on stateful tables.
    // FIXME -- there are stats_vpn_range regs that could work for counters on Jbay (but
    // no stats_vpn_offset to adjust, and no support in the assembler yet).  stateful
    // address to adb path looks like it might be generic for meter address, so could be
    // used for meters as well?  Also no assembler support yet.
    return false;
}

bool TablePlacement::initial_stage_and_entries(Placed *rv, int &furthest_stage) {
    // TODO: In the second round of table placement, where we ignore container conflicts,
    // we probably should not ignore container conflicts that are unavoidable because of PARDE
    // constraints. However, this feature is causing some regressions because in PHV metadata
    // initialization, we might add initializations that would introduce container conflicts
    // for other fields in the same byte with it. This feature will be enabled with the fix
    // on metadata initialization side.
    const bool enable_unavoidable_container_conflict = false;
    rv->use.format_type.invalidate();  // will need to be recomputed
    auto *t = rv->table;
    if (t->match_table) {
        rv->entries = 512;  // default number of entries -- FIXME does this really make sense?
        if (t->layout.no_match_data())
            rv->entries = 1;
        if (t->layout.pre_classifier)
            rv->entries = t->layout.pre_classifer_number_entries;
        else if (auto k = t->match_table->getConstantProperty("size"_cs))
            rv->entries = k->asInt();
        else if (auto k = t->match_table->getConstantProperty("min_size"_cs))
            rv->entries = k->asInt();
        if (t->layout.has_range) {
            RangeEntries re(phv, rv->entries);
            t->apply(re);
            rv->entries = re.TCAM_lines();
        } else if (t->layout.alpm && t->layout.atcam) {
            // Alpm requires one extra entry per partition. This is a driver
            // requirement. The extra entry per partition allows driver to
            // support atomic modifies of the entry.  'disable_atomic_modify'
            // annotation can be used to disable this if modifies will not
            // happen or do not need to happen atomically.
            //
            // Atomic modifies require driver to move the entry from one
            // location to another location if we are changing multiple RAMs
            // such as the VLIW instruction stored in match overhead and the
            // action data stored in a direct action table.
            //
            // Atomic modify is supported on Tofino2 through hardware and does
            // not require additinal entries.
            bool disable_atomic_modify;
            t->getAnnotation("disable_atomic_modify"_cs, disable_atomic_modify);

            // NOTE: Replace with commented code once driver support is in for
            // Tofino2+ archs
            // if (!disable_atomic_modify && BackendOptions().target == "tofino")
            //     rv->entries += t->layout.partition_count;
            // Similar check in mau/asm_output.cpp -> emit_table_context_json()
            if (!(disable_atomic_modify && BackendOptions().target == "tofino"))
                rv->entries += t->layout.partition_count;
        }

        if (t->layout.exact) {
            if (t->layout.ixbar_width_bits < ceil_log2(rv->entries)) {
                rv->entries = 1 << t->layout.ixbar_width_bits;
                // Skip warnings on compiler generated tables
                if (!t->is_compiler_generated)
                    warning(BFN::ErrorType::WARN_TABLE_PLACEMENT,
                              "Shrinking %1%: with %2% match bits, can only have %3% entries",
                              t, t->layout.ixbar_width_bits, rv->entries);
            }
        }
    } else {
        rv->entries = 0;
    }
    /* Not yet placed tables that share an attached table with this table -- if any of them
     * have a dependency that prevents placement in the current stage, we want to defer */
    ordered_set<const IR::MAU::Table *> tables_with_shared;
    for (auto *ba : rv->table->attached) {
        if (!rv->attached_entries.emplace(ba->attached, ba->attached->size).second)
            BUG("%s attached more than once", ba->attached);
        if (ba->attached->direct || can_duplicate(ba->attached)) continue;
        bool stateful_selector = ba->attached->is<IR::MAU::StatefulAlu>() &&
                                 ba->use == IR::MAU::StatefulUse::NO_USE;
        for (auto *att_to : attached_to.at(ba->attached)) {
            if (att_to == rv->table) continue;
            // If shared with another table that is not placed yet, need to
            // defer the placement of this attached table
            if (!rv->is_match_placed(att_to)) {
                tables_with_shared.insert(att_to);

                // Can't split indirect attached table when more than one stateful action exist
                if (count_sful_actions(rv->table) > 1)
                    continue;

                // Cannot split attached entries for Tofino
                if (Device::currentDevice() == Device::TOFINO) continue;

                rv->attached_entries.at(ba->attached).entries = 0;
                rv->attached_entries.at(ba->attached).need_more = true;
                if (stateful_selector) {
                    // A Register that is not directly used, but is instead the backing for a
                    // selector.  Since a selector cannot be split from its match table, we
                    // don't want to try to place the table until all the tables that write
                    // to it are placed.
                    auto *att_ba = att_to->get_attached(ba->attached);
                    BUG_CHECK(att_ba, "%s not attached to %s?", ba->attached, att_to);
                    if (att_ba->use != IR::MAU::StatefulUse::NO_USE)
                        return false;
                } else {
                    break; } } } }

    int prev_stage_tables = 0;
    auto init_attached = rv->attached_entries;
    std::set<const IR::MAU::AttachedMemory *> need_defer;
    for (auto *p = rv->prev; p; p = p->prev) {
        LOG5("For previous table : " << p->name);
        if (p->name == rv->name) {
            LOG1("To place table : " << rv->name << ", entries: " << rv->entries
                    << ", to place entries: " << p->entries);
            if (p->need_more == false) {
                BUG(" - can't place %s in stage %d it's already done", rv->name, rv->stage);
                return false;
            }
            rv->entries -= p->entries;
            for (auto &ate : p->attached_entries) {
                if (!ate.first->direct && can_duplicate(ate.first) &&
                    init_attached.at(ate.first).entries <= ate.second.entries) {
                    // if the attached table will be duplicated in every stage, we want to
                    // treat every stage as if it is the first&last stage (so no need to get
                    // an address from an earlier stage or send it to a later stage)
                    continue; }
                if (ate.second.entries > 0 || !ate.second.first_stage)
                    rv->attached_entries.at(ate.first).first_stage = false;
                if (ate.first->direct) continue;
                rv->attached_entries.at(ate.first).entries -= ate.second.entries;
                // if the match table is split, can't currently put any of the indirect
                // attached tables in the same stage as (part of) the match, so need to
                // move all entries to a later stage UNLESS we're finished with the match
                need_defer.insert(ate.first); }
            prev_stage_tables++;
            if (p->stage == rv->stage) {
                LOG2("  Cannot place multiple sections of an individual table in the same stage");
                rv->stage_advance_log = "cannot split into same stage"_cs;
                rv->stage++; }
        } else if (p->stage == rv->stage) {
            if (options.forced_placement)
                continue;
            if (deps.happens_phys_before(p->table, rv->table)) {
                rv->stage++;
                LOG2("  - dependency between " << p->table->name << " and table advances stage");
                rv->stage_advance_log = "dependency on table " + p->table->name;
            } else if (rv->gw && deps.happens_phys_before(p->table, rv->gw)) {
                rv->stage++;
                LOG2("  - dependency between " << p->table->name << " and gateway advances stage");
                rv->stage_advance_log = "gateway dependency on table " + p->table->name;
            } else if (p->gw && deps.happens_phys_before(p->gw, rv->table)) {
                rv->stage++;
                LOG2("  - dependency between " << p->gw->name << " and table advances stage");
                rv->stage_advance_log = "separate gateway dependency on table " + p->table->name;
            } else if ((!ignoreContainerConflicts &&
                        deps.container_conflict(p->table, rv->table)) ||
                       (enable_unavoidable_container_conflict && ignoreContainerConflicts &&
                        deps.unavoidable_container_conflict(p->table, rv->table))) {
                rv->stage++;
                LOG2("  - action dependency between "
                     << p->table->name << " and table " << rv->table->name
                     << " due to PHV allocation advances stage to " << rv->stage);
                rv->stage_advance_log = "container conflict with table " + p->table->name;
            } else if (not_eligible.count(rv->table)) {
                rv->stage++;
                LOG2("  - table " << rv->table->name << " not eligible");
                rv->stage_advance_log = "grouping conflict with table " + rv->table->name;
            } else {
                for (auto ctbl : tables_with_shared) {
                    // Validate all physical dependency of shared table and make sure they are all
                    // cleared before setting the position in the next stage
                    auto &pre_tbls = deps.happens_phys_after_map[ctbl];
                    for (auto pre_tbl : pre_tbls) {
                        if (!rv->is_placed(pre_tbl)) {
                            LOG2("  - dependency between " << pre_tbl->name << " and " <<
                                 ctbl->name << " requiring more than one stage");
                            return false;
                        }
                    }
                    if (deps.happens_phys_before(p->table, ctbl)) {
                        rv->stage++;
                        LOG2("  - dependency between " << p->table->name << " and " <<
                             ctbl->name << " advances stage");
                        rv->stage_advance_log = "shared table " + ctbl->name +
                            " depends on table " + p->table->name;
                        break;
                    } else if ((!ignoreContainerConflicts &&
                                deps.container_conflict(p->table, ctbl)) ||
                               (enable_unavoidable_container_conflict && ignoreContainerConflicts &&
                                deps.unavoidable_container_conflict(p->table, ctbl))) {
                        rv->stage++;
                        LOG2("  - action dependency between " << p->table->name << " and "
                             "table " << ctbl->name << " due to PHV allocation advances "
                             "stage to " << rv->stage);
                        rv->stage_advance_log = "shared table " + ctbl->name +
                            " container conflict with table " + p->table->name;
                        break;
                    }
                }
            }
        }
    }
    if (rv->entries <= 0 && !t->conditional_gateway_only() && !t->is_always_run_action()) {
        rv->entries = 0;
        // FIXME -- should use std::any_of, but pre C++-17 is too hard to use and verbose
        bool have_attached = false;
        for (auto &ate : rv->attached_entries) {
            if (ate.second.entries > 0) {
                have_attached = true;
                break; } }
        if (!have_attached) return false;
    } else {
        for (auto *at : need_defer) {
            // if the match table is split, can't currently put any of the indirect
            // attached tables in the same stage as (part of) the match
            rv->attached_entries.at(at).need_more = rv->attached_entries.at(at).entries > 0;
            rv->attached_entries.at(at).entries = 0; } }

    auto stage_pragma = t->get_provided_stage(rv->stage, &rv->requested_stage_entries,
                                              &rv->stage_flags);
    if (rv->requested_stage_entries > 0)
        LOG5("Using " << rv->requested_stage_entries << " for stage " << rv->stage
             << " out of total " << rv->entries);

    if (stage_pragma >= 0) {
        rv->stage = std::max(stage_pragma, rv->stage);
        furthest_stage = std::max(rv->stage, furthest_stage);
    } else if (options.forced_placement && !t->conditional_gateway_only()) {
        warning("%s: Table %s has not been provided a stage even though forced placement of "
                  "tables is turned on", t->srcInfo, t->name);
    }

    if (prev_stage_tables > 0) {
        rv->stage_split = prev_stage_tables;
    }
    for (auto *ba : rv->table->attached) {
        if (ba->attached->direct)
            rv->attached_entries.at(ba->attached).entries = rv->entries; }

    return true;
}

/** When placing a gateway table, the gateway can potential be combined with a match table
 *  to build one logical table.  The gateway_merge function picks a legal table to place with
 *  this table.  This loops through all possible tables to be merged, and will
 *  return a vector of these possible choices to the is_better function to choose
 */
safe_vector<TablePlacement::Placed *>
    TablePlacement::try_place_table(const IR::MAU::Table *t, const Placed *done,
        const StageUseEstimate &current, GatewayMergeChoices& gmc,
        const TableSummary::PlacedTable *pt) {
    Log::TempIndent indent;
    LOG1("try_place_table(" << t->name << ", stage=" << (done ? done->stage : 0) << ")" << indent);
    safe_vector<Placed *> rv_vec;
    // Place and save a placement, as a lambda
    auto try_place = [&](Placed* rv) {
                         if ((rv = try_place_table(rv, current, pt)))
                             rv_vec.push_back(rv);
                     };

    // If we're not a gateway or there are no merge options for the gateway, we create exactly one
    // placement for it
    if (!t->uses_gateway() || t->match_table || gmc.size() == 0) {
        auto *rv = new Placed(*this, t, done);
        try_place(rv);
    } else {
        // Otherwise, we are a gateway, so we need to iterate through all tables it can possibly be
        // merged with
        for (auto mc : gmc) {
            auto *rv = new Placed(*this, t, done);
            // Merge
            LOG1("  Merging with match table " << mc.first->name << " and tag " << mc.second);
            rv->gateway_merge(mc.first, mc.second);
            // Get a placement
            try_place(rv);
        }
    }
    return rv_vec;
}


TablePlacement::Placed *TablePlacement::try_place_table(Placed *rv,
        const StageUseEstimate &current, const TableSummary::PlacedTable *pt) {
    int furthest_stage = (rv->prev == nullptr) ? 0 : rv->prev->stage + 1;
    if (!initial_stage_and_entries(rv, furthest_stage)) {
        return nullptr;
    }

    int needed_entries = rv->entries;
    attached_entries_t initial_attached_entries = rv->attached_entries;

    // Setup stage and entries for Alt Table Placement Round
    if (summary.is_table_replay() && pt) {
        furthest_stage = pt->stage;
        rv->stage = pt->stage;

        // Within a table replay round, table placement is replayed to allocate smaller of these 2
        // values:
        // 1. Same entries as last round
        // 2. Minimum requested entries (via p4 table size / split scenarios etc)
        //
        // Sometimes the compiler overallocates entries based on packing. The
        // replay should only try to allocate minimum required entries on the table and avoid any
        // unnecessary overallocation as this may limit resources for other tables in stage or cause
        // the current table to not find an allocation.
        //
        // E.g. ipv4_acl has a table size of 4096 (4K) entries in P4
        // Table Placement Initial Round -> 4 pack x 4 ways = 16K entries
        // Table Placement Replay Round  -> 1 pack x 4 ways =  4K entries
        //
        // Replay round should __not__ try to allocate 16K entries but the requested 4K as that is
        // the minimum requested on the table. Previous round caused overallocation due to its
        // unique scenario and packing decisiongs which may not hold true for this round. There is a
        // possibility it wont be able to pack 4 entries in the word and end up requiring more SRAMs
        // than necessary.
        rv->entries = std::min(pt->entries, rv->entries);

        // If container conflicts skip being accounted for in first TP round
        // exit this round. This round follows placement in round 1 with the
        // assumption that all dependencies are considered. Ideally this should
        // be figured out by trivial alloc so previous round does not generate
        // an invalid allocation. The check below will simply exit but a
        // BUG_CHECK here might help identify the skipped conflicts to improve
        // trivial alloc.
        for (auto *p = rv->prev; p; p = p->prev) {
            if (deps.container_conflict(p->table, rv->table)
                    && p->stage == rv->stage
                    && p->name != rv->name) {
                error("Table placement for same ordering will exit as an "
                    "unallocated container conflict has been detected causing tables "
                    "%s and %s being placed in same stage %d", p->name, rv->name,
                    rv->stage);
                return nullptr;
            }
        }
        for (auto &batt : rv->table->attached) {
            for (auto ptAtt : pt->attached_entries) {
                if (batt->attached->name == ptAtt.first) {
                    rv->attached_entries.emplace(batt->attached, ptAtt.second);
                    break;
                }
            }
        }
    }
    const Placed *done = rv->prev;
    std::vector<Placed *> whole_stage;
    error_message = ""_cs;
    // clone the already-placed tables in this stage so they can be re-placed
    for (const Placed **p = &rv->prev; *p && (*p)->stage == rv->stage; ) {
        auto clone = new Placed(**p);
        whole_stage.push_back(clone);
        *p = clone;
        p = &clone->prev; }

    // update shared attached tables in the stage
    for (auto *p : boost::adaptors::reverse(whole_stage))
        p->update_attached(rv);
    if (!whole_stage.empty()) {
        BUG_CHECK(rv->prev && rv->prev->stage == rv->stage, "whole_placed invalid");
        rv->placed |= rv->prev->placed;
        BUG_CHECK(rv->match_placed == rv->prev->match_placed, "match_placed out of date?"); }

    int initial_entries = rv->requested_stage_entries > 0 ? rv->requested_stage_entries :
        rv->entries;
    int initial_stage_split = rv->stage_split;

    LOG3("  Initial # of stages is " << rv->stage << ", initial # of entries is " << rv->entries);
    LOG4("    furthest stage: " << furthest_stage << ", initial # of attached entries: "
            << initial_attached_entries);

    if (rv->stage >= 100)  ::fatal_error("too many stages: %1", rv->stage);

    auto *min_placed = new Placed(*rv);
    if (min_placed->entries > 1)
        min_placed->entries = 1;
    if (min_placed->requested_stage_entries > 0)
        min_placed->entries = rv->requested_stage_entries;
    if (count_sful_actions(rv->table) > 1) {
        // FIXME -- can't currently split tables with multiple stateful actions, as doing so
        // would require passing the meter_type via tempvar somehow.
    } else {
        for (auto &[ att_mem, att_entries ] : min_placed->attached_entries) {
            if (att_mem->direct) continue;
            if (att_entries.entries == 0) continue;
            if (can_split(min_placed->table, att_mem)) {
                if (needed_entries == 0) {
                    auto min_split_size = att_mem->get_min_size();
                    if (att_entries.entries > min_split_size) {
                        att_entries.entries = min_split_size;
                        att_entries.need_more = true;
                    }
                } else {
                    att_entries.entries = 0;
                    att_entries.need_more = true;
                }
            }
        }
    }

    if (!rv->table->created_during_tp) {
        BUG_CHECK(!rv->placed[uid(rv->table)],
                  "try_place_table(%s, stage = %d) when it is already placed?", rv->name,
                  rv->stage);
    }

    int initial_min_placed_entries = min_placed->entries;
    attached_entries_t initial_min_placed_attached_entries = min_placed->attached_entries;

    StageUseEstimate stage_current = current;
    // According to the driver team, different stage tables can have different action
    // data allocations, so the algorithm doesn't have to prefer this allocation across
    // stages

    TableSummary::PlacementResult allocated = TableSummary::FAIL;

    if (rv->prev && rv->stage != rv->prev->stage)
        stage_current.clear();

    /* Loop to find the right size of entries for a table to place into stage */
    do {
        rv->entries = initial_entries;
        rv->attached_entries = initial_attached_entries;

        min_placed->entries = initial_min_placed_entries;
        min_placed->attached_entries = initial_min_placed_attached_entries;

        auto avail = StageUseEstimate::max();
        bool advance_to_next_stage = false;
        allocated = TableSummary::FAIL;
        // Rebuild the layout when moving from stage to stage
        rv->use.format_type.invalidate();
        min_placed->use.format_type.invalidate();

        // Try to allocate the entire table
        do {
            allocated = try_alloc_all(rv, whole_stage, "Table use");
            if (allocated == TableSummary::SUCC)
                break;
            rv->use.preferred_index++;
        } while (rv->use.layout_options.size() &&
                (rv->use.preferred_index < rv->use.layout_options.size()));

        if (allocated == TableSummary::SUCC)
            break;

        // If a table contains initialization for dark containers, it cannot be split into
        // multiple stages. Ignore this limitation if the table allocation is run with ignore
        // container conflict. For this particular case PHV will always rerun and ultimately
        // catch this situation and avoid dark initialization on this splitted table.
        if (rv->table->has_dark_init) {
            LOG3("    Table with dark initialization cannot be split");
            if (!ignoreContainerConflicts) {
                error_message = cstring("PHV allocation doesn't want this table split, and it's "
                                        "too big for one stage");
                advance_to_next_stage = true;
            }
        }

        // Not able to allocate the entire table, try the smallest possible size
        if (!advance_to_next_stage) {
            TableSummary::PlacementResult min_allocated = TableSummary::FAIL;
            do {
                min_allocated = try_alloc_all(min_placed, whole_stage, "Min use");
                if (min_allocated == TableSummary::SUCC)
                    break;
                min_placed->use.preferred_index++;
            } while (min_placed->use.layout_options.size() &&
                    (min_placed->use.preferred_index < min_placed->use.layout_options.size()));

            if (min_allocated != TableSummary::SUCC) {
                if (!(rv->stage_advance_log = min_placed->stage_advance_log))
                    rv->stage_advance_log = "repacking previously placed failed"_cs;
                advance_to_next_stage = true;
            }
        }
        if (!advance_to_next_stage) {
            // Now try to shrink the table to maximize the number of entries that can fit into
            // this particular stage
            if (rv->prev && rv->stage == rv->prev->stage) {
                avail.srams -= stage_current.srams;
                avail.tcams -= stage_current.tcams;
                avail.maprams -= stage_current.maprams;
            }

            int srams_left = avail.srams;
            int tcams_left = avail.tcams;
            if (!shrink_estimate(rv, srams_left, tcams_left, min_placed->entries)) {
                if (!error_message)
                    error_message = cstring("Can't split this table across stages and it's "
                                            "too big for one stage");
                advance_to_next_stage = true; }
            while (!advance_to_next_stage) {
                rv->update_need_more(needed_entries);
                // If the table is split for the first time, then the stage_split is set to 0
                if (rv->need_more && initial_stage_split == -1)
                    rv->stage_split = 0;
                // If the table does not need more entries and is previously marked
                // as split for the first time, then reset the stage_split to -1
                // Note: Stage split value is used during allocation to create
                // unique id suffixes e.g. $st0, $st1 etc. hence if this is
                // incorrectly set the unique ids generated in memories.cpp will not
                // match those generated on the table
                else if (!rv->need_more && rv->stage_split == 0)
                    rv->stage_split = -1;

                allocated = try_alloc_all(rv, whole_stage, "Table shrink");
                if (allocated == TableSummary::SUCC)
                    break;

                // Shrink preferred layout and sort them by prioritizing the number of entries
                if (!shrink_preferred_lo(rv)) {
                    error_message = "Can't shrink layout anymore"_cs;
                    advance_to_next_stage = true;
                    break;
                }
            }
        }
        if (advance_to_next_stage) {
            rv->stage++;
            rv->stage_split = initial_stage_split;
            rv->use.preferred_index = 0;
            min_placed->stage++;
            min_placed->stage_split = initial_stage_split;
            min_placed->use.preferred_index = 0;
            if (done) {
                rv->placed -= rv->prev->placed - done->placed;
                min_placed->placed -= min_placed->prev->placed - done->placed; }
            rv->prev = min_placed->prev = done;
            stage_current.clear();
            for (auto *p : whole_stage) delete p;  // help garbage collector
            whole_stage.clear(); }
    } while (allocated != TableSummary::SUCC && rv->stage <= furthest_stage);

    rv->update_need_more(needed_entries);
    rv->update_formats();
    for (auto *t : whole_stage)
        t->update_formats();

    LOG3("  Selected stage: " << rv->stage << "    Furthest stage: " << furthest_stage);
    if (rv->stage > furthest_stage) {
        if (error_message == "")
            error_message = "Unknown error for stage advancement?"_cs;
        error("Could not place %s: %s", rv->table, error_message);
        summary.set_table_replay_result(allocated);
        return nullptr;
    }

    if (!rv->table->conditional_gateway_only()) {
        BUG_CHECK(rv->use.preferred_action_format() != nullptr,
                  "Action format could not be found for a particular layout option.");
        BUG_CHECK(rv->use.preferred_meter_format() != nullptr,
                  "Meter format could not be found for a particular layout option."); }

    rv->setup_logical_id();

    if (!rv->table->is_always_run_action())
       BUG_CHECK((rv->logical_id / StageUse::MAX_LOGICAL_IDS) == rv->stage, "Table %s is not "
                 "assigned to the same stage (%d) at its logical id (%d)",
                 rv->table->externalName(), rv->stage, rv->logical_id);
    LOG2("  try_place_table returning " << rv->entries << " of " << rv->name <<
         " in stage " << rv->stage <<
         (rv->need_more_match ? " (need more match)" : rv->need_more ? " (need more)" : ""));
    // LOG5(IndentCtl::indent << IndentCtl::indent << "    " <<
    //     rv->resources <<
    //     IndentCtl::unindent << IndentCtl::unindent);

    if (!rv->table->created_during_tp) {
        if (!rv->need_more_match) {
            rv->match_placed[uid(rv->table)] = true;
            if (!rv->need_more)
                rv->placed[uid(rv->table)] = true;
        }
        LOG3("Table is " << (rv->placed[uid(rv->table)] ? "" : "not") << " placed");
        if (rv->gw) {
            rv->match_placed[uid(rv->gw)] = true;
            rv->placed[uid(rv->gw)] = true;
            LOG3("Gateway is " << (rv->placed[uid(rv->gw)] ? "" : "not") << " placed");
        }
    }

    // Update layout option used for this placement
    if (auto *layout_option = rv->use.preferred())
        rv->resources.layout_option = *layout_option;

    return rv;
}

/**
 * Try to backfill a table in the current stage just before another table, bumping up the
 * logical ids of the later tables, but keeping all in the same stage.
 */
TablePlacement::Placed *DecidePlacement::try_backfill_table(
        const Placed *done, const IR::MAU::Table *tbl, cstring before) {
    LOG2("try to backfill " << tbl->name << " before " << before);
    std::vector<Placed *> whole_stage;
    Placed *place_before = nullptr;
    for (const Placed **p = &done; *p && (*p)->stage == done->stage; ) {
        if (!(*p)->table->created_during_tp && !self.mutex(tbl, (*p)->table) &&
            self.deps.container_conflict(tbl, (*p)->table)) {
            LOG4("  can't backfill due to container conflict with " << (*p)->name);
            return nullptr; }
        auto clone = new Placed(**p);
        whole_stage.push_back(clone);
        if (clone->name == before) {
            BUG_CHECK(!place_before, "%s placed multiple times in stage %d", before, done->stage);
            place_before = clone; }
        *p = clone;
        p = &clone->prev; }
    if (!place_before) {
        BUG("Couldn't find %s in stage %d", before, done->stage);
        return nullptr; }
    Placed *pl = new Placed(self, tbl, place_before->prev);
    int furthest_stage = done->stage;
    if (!self.initial_stage_and_entries(pl, furthest_stage))
        return nullptr;
    if (pl->stage != place_before->stage)
        return nullptr;
    place_before->prev = pl;
    if (self.try_alloc_all(pl, whole_stage, "Backfill") != TableSummary::SUCC)
        return nullptr;
    for (auto &ae : pl->attached_entries)
        if (ae.second.entries == 0 || ae.second.need_more)
            return nullptr;
    for (auto *p : whole_stage) {
        p->placed[self.uid(tbl)] = 1;
        p->match_placed[self.uid(tbl)] = 1;
        if (p == place_before)
            break; }
    pl->update_formats();
    for (auto *p : whole_stage)
        p->update_formats();
    // The new allocation may change how many logical tables the already placed tables require,
    // so we recalculate logical IDs for every table in the stage.
    for (auto *p : boost::adaptors::reverse(whole_stage)) {
        if (p->prev == pl) pl->setup_logical_id();
        p->setup_logical_id();
        if (p->logical_id / StageUse::MAX_LOGICAL_IDS != p->stage) return nullptr;
    }
    pl->placed[self.uid(tbl)] = 1;
    pl->match_placed[self.uid(tbl)] = 1;
    auto tbl_log_str =
        pl->table ? pl->name + " ( " + pl->table->externalName() + " ) " : pl->name;
    auto gw_log_str = pl->gw ?
        " (with gw " + pl->gw->name + ", result tag " + pl->gw_result_tag + ")" : "";
    LOG1("placing " << pl->entries << " entries of " << tbl_log_str << gw_log_str
                    << " in stage " << pl->stage << "(" << hex(pl->logical_id) << ") "
                    << pl->use.format_type << " (backfilled)");
    BUG_CHECK(pl->table->next.empty(), "Can't backfill table with control dependencies");
    return whole_stage.front();
}

/**
 * In Tofino specifically, if a table is in ingress or egress, then a table for that pipeline
 * must be placed within stage 0.  The parser requires a pathway into the first stage.
 *
 * Thus, this checks if no table can longer be placed in stage 0, and if a table from a
 * particular gress has not yet been placed, the create a starting table for that pipe in
 * stage 0.
 */
const TablePlacement::Placed *TablePlacement::add_starter_pistols(const Placed *done,
                                                                  const Placed **best,
                                                                  const StageUseEstimate &current) {
    if (Device::currentDevice() != Device::TOFINO)
        return done;
    if (done != nullptr && done->stage > 0)
        return done;
    if (best != nullptr && *best != nullptr && (*best)->stage == 0)
        return done;

    // Determine if a table has been placed yet within this gress
    std::array<bool, 2> placed_gress = { { false, false } };
    for (auto *p = done; p && p->stage == 0; p = p->prev) {
        placed_gress[p->table->gress] = true;
    }

    const Placed *last_placed = done;
    for (int i = 0; i < 2; i++) {
        gress_t current_gress = static_cast<gress_t>(i);
        if (!placed_gress[i] && table_in_gress[i]) {
            cstring t_name = "$"_cs + toString(current_gress) + "_starter_pistol"_cs;
            auto t = new IR::MAU::Table(t_name, current_gress);
            t->created_during_tp = true;
            LOG4("Adding starter pistol for " << current_gress);
            auto *rv = new Placed(*this, t, last_placed);
            rv = try_place_table(rv, current);
            if (!rv || rv->stage != 0) {
                error("No table in %s could be placed in stage 0, a requirement for Tofino",
                      toString(current_gress));
                return last_placed;
            }
            last_placed = rv;
            starter_pistol[i] = t;
        }
    }

    // place_table cannot be called on these tables, as they don't appear in the initial IR.
    // Adding them to the placed linked list is sufficient for them to be placed
    if (best != nullptr && *best != nullptr) *best = (*best)->diff_prev(last_placed);
    return last_placed;
}

const TablePlacement::Placed *
DecidePlacement::place_table(ordered_set<const GroupPlace *>&work, const Placed *pl) {
    if (LOGGING(1)) {
        auto tbl_log_str =
            pl->table ? pl->name + " ( " + pl->table->externalName() + " ) " : pl->name;
        auto gw_log_str = pl->gw ?
            " (with gw " + pl->gw->name + ", result tag " + pl->gw_result_tag + ")" : "";
        auto need_more_match_str = pl->need_more_match ?
            " (need more match)" : pl->need_more ? " (need more)" : "";
        LOG1("placing " << pl->entries << " entries of " << tbl_log_str << gw_log_str
                        << " in stage " << pl->stage << "(" << hex(pl->logical_id) << ") "
                        << pl->use.format_type << need_more_match_str);
    }

    CHECK_NULL(pl->table);

    int dep_chain = self.deps.stage_info[pl->table].dep_stages_control_anti;
    if (pl->stage + dep_chain >= Device::numStages())
        LOG1(" Dependence chain (" << pl->stage + dep_chain + 1
                                   << ") longer than available stages (" << Device::numStages()
                                   << ")");
    int stage_pragma = pl->table->get_provided_stage(pl->stage);
    if (stage_pragma >= 0 && stage_pragma != pl->stage)
        LOG1("  placing in stage " << pl->stage << " despite @stage(" << stage_pragma << ")");

    if (!pl->need_more) {
        pl->group->finish_if_placed(work, pl); }
    GroupPlace *gw_match_grp = nullptr;
    /**
     * A gateway cannot be linked with a table that is applied multiple times in different way.
     * Take for instance the following program:
     *
     *     if (t1.apply().hit) {
     *         if (f1 == 0) {   // cond-0
     *             t2.apply();
     *         }
     *     } else {
     *         if (f1 == 1) {
     *             t2.apply();   // cond-1
     *         }
     *     }
     *
     * In this case, table t2 cannot be linked to either of the gateway tables, as by linking
     * a gateway to one would lose the value from others.  This code verifies that a table
     * linked to a gateway is not breaking this constraint.
     */
    if (pl->gw)  {
        bool found_match = false;
        for (auto n : Values(pl->gw->next)) {
            if (!n || n->tables.size() == 0) continue;
            if (GroupPlace::in_work(work, n)) continue;
            bool ready = true;
            // Vector of all control parents of a TableSeq.  In our example, if placing cond-0,
            // the sequence would be [ t2 ], and the parent of that sequence would be
            // [ cond-0, cond-1 ]
            ordered_set<const GroupPlace *> parents;
            for (auto tbl : self.seqInfo.at(n).refs) {
                if (pl->is_placed(tbl)) {
                    parents.insert(pl->find_group(tbl));
                } else {
                    ready = false;
                    break; } }
            if (n->tables.size() == 1 && n->tables.at(0) == pl->table) {
                BUG_CHECK(!found_match && !gw_match_grp,
                          "Table appears twice: %s", pl->table->name);
                // Guaranteeing at most only one parent for linking a gateway
                BUG_CHECK(ready && parents.size() == 1, "Gateway incorrectly placed on "
                          "multi-referenced table");
                found_match = true;
                if (pl->need_more)
                    new GroupPlace(*this, work, parents, n);
                continue; }
            // If both cond-1 and cond-0 are placed, then the sequence for t2 can be placed
            GroupPlace *g = ready ? new GroupPlace(*this, work, parents, n) : nullptr;
            // This is based on the assumption that a table appears in a single table sequence
            for (auto t : n->tables) {
                if (t == pl->table) {
                    BUG_CHECK(!found_match && !gw_match_grp,
                              "Table appears twice: %s", t->name);
                    BUG_CHECK(ready && parents.size() == 1, "Gateway incorrectly placed on "
                              "multi-referenced table");
                    found_match = true;
                    gw_match_grp = g; } } }
        BUG_CHECK(found_match, "Failed to find match table"); }
    if (!pl->need_more_match) {
        for (auto n : Values(pl->table->next)) {
            if (n && n->tables.size() > 0 && !GroupPlace::in_work(work, n)) {
                bool ready = true;
                ordered_set<const GroupPlace *> parents;
               /**
                * Examine the following example:
                *
                *     if (condition) {
                *         switch(t1.apply().action_run) {
                *             a1 : { t3.apply(); }
                *             a2 : { if (t2.apply().hit) { t3.apply(); } }
                *         }
                *     }
                *
                * The algorithm wants to merge the condition with t1.  The tables that become
                * available to place would be t2 and t3.  However, we do not want to add the
                * t3 sequence yet to the algorithm, as it has to wait
                */
                for (auto tbl : self.seqInfo.at(n).refs) {
                    if (tbl == pl->table) {
                        parents.insert(gw_match_grp ? gw_match_grp : pl->group);
                    } else if (pl->is_placed(tbl)) {
                        // After analysis, inserting a
                        // parent already placed with a different group should be supported.
                        // BUG_CHECK(!gw_match_grp, "Failure attaching gateway to table");
                        parents.insert(pl->find_group(tbl));
                    } else {
                        // Basically when placing t1, which is a parent of t3, do not begin
                        // to make t3 an available sequence, as it has to wait.
                        ready = false;
                        break ; } }
                if (ready && pl->need_more) {
                    for (auto t : n->tables) {
                        if (!can_place_with_partly_placed(t, {pl->table}, pl)) {
                            ready = false;
                            break; } } }
                if (ready) {
                    new GroupPlace(*this, work, parents, n); } } } }
    return pl;
}

bool DecidePlacement::are_metadata_deps_satisfied(const Placed *placed,
                                                 const IR::MAU::Table* t) const {
    // If there are no reverse metadata deps for this table, return true.
    LOG4("Checking table " << t->name << " for metadata dependencies");
    const ordered_set<cstring> set_of_tables = self.phv.getReverseMetadataDeps(t);
    for (auto tbl : set_of_tables) {
        if (!placed || !placed->is_placed(tbl)) {
            LOG4("    Table " << tbl << " needs to be placed before table " << t->name);
            return false;
        }
    }
    return true;
}

int DecidePlacement::get_control_anti_split_adj_score(const Placed *pl) const {
    int init_stage = pl->init_stage();
    int dep_s = self.deps.stage_info[pl->table].dep_stages_control_anti_split;
    int dep_ns = self.deps.stage_info[pl->table].dep_stages_control_anti;
    int delta_stages = pl->stage - init_stage;
    return std::max(dep_ns, dep_s - delta_stages - 1);
}

/* compare two tables to see which one we should prefer placindg next.  Return true if
 * a is better and false if b is better */
bool DecidePlacement::is_better(const Placed *a, const Placed *b,
                                TablePlacement::choice_t& choice) {
    const IR::MAU::Table *a_table_to_use = a->gw ? a->gw : a->table;
    const IR::MAU::Table *b_table_to_use = b->gw ? b->gw : b->table;

    if (a_table_to_use == b_table_to_use) {
        a_table_to_use = a->table;
        b_table_to_use = b->table;
    }

    // FIXME -- which state should we use for this -- a->prev or b->prev?  Currently they'll
    // be the same (as far as placed tables are concerned) as we only try_place a single table
    // before committing, but in the future...
    BUG_CHECK(a->prev == b->prev || a->prev->match_placed == b->prev->match_placed,
              "Inconsistent previously placed state in is_better");
    const Placed *done = a->prev;
    self.ddm.update_placed_tables([done](const IR::MAU::Table *tbl)->bool {
        return done ? done->is_match_placed(tbl) : false; });
    std::pair<int, int> down_score;
    // dynamic downward propagation compute only make sense on Tofino1 but keep it on non resource
    // mode to keep the same behaviour on legacy program.
    if (Device::currentDevice() == Device::TOFINO || !resource_mode) {
        down_score = self.ddm.get_downward_prop_score(a_table_to_use, b_table_to_use);
        // Tofino1 under resource mode...
        if (Device::currentDevice() == Device::TOFINO) {
            down_score.first = std::max(down_score.first, get_control_anti_split_adj_score(a));
            down_score.second = std::max(down_score.second, get_control_anti_split_adj_score(b));
        }
    } else {
        down_score = std::make_pair(get_control_anti_split_adj_score(a),
                                    get_control_anti_split_adj_score(b));
    }

    ordered_set<const IR::MAU::Table *> already_placed_a;
    for (auto p = a->prev; p && p->stage == a->stage; p = p->prev) {
        if (p->table)
            already_placed_a.emplace(p->table);
        if (p->gw)
            already_placed_a.emplace(p->gw);
    }

    ordered_set<const IR::MAU::Table *> already_placed_b;
    for (auto p = b->prev; p && p->stage == b->stage; p = p->prev) {
        if (p->table) {
            already_placed_b.emplace(p->table);
        }
        if (p->gw)
            already_placed_b.emplace(p->gw);
    }

    LOG5("      Stage A is " << a->name << ((a->gw) ? (" $" + a->gw->name) : "") <<
         " with calculated stage " << a->stage <<
         ", provided stage " << a->table->get_provided_stage(a->stage) <<
         ", priority " << a->table->get_placement_priority_int());
    LOG5("        downward prop score " << down_score.first);
    LOG5("        local dep score "
          << self.deps.stage_info.at(a_table_to_use).dep_stages_control_anti);
    LOG5("        dom frontier "
          << self.deps.stage_info.at(a_table_to_use).dep_stages_dom_frontier);
    LOG5("        can place cds in stage "
          << self.ddm.can_place_cds_in_stage(a_table_to_use, already_placed_a));

    LOG5("      Stage B is " << b->name << ((a->gw) ? (" $" + a->gw->name) : "") <<
         " with calculated stage " << b->stage <<
         ", provided stage " << b->table->get_provided_stage(b->stage) <<
         ", priority " << b->table->get_placement_priority_int());
    LOG5("        downward prop score " << down_score.second);
    LOG5("        local dep score "
          << self.deps.stage_info.at(b_table_to_use).dep_stages_control_anti);
    LOG5("        dom frontier "
          << self.deps.stage_info.at(b_table_to_use).dep_stages_dom_frontier);
    LOG5("        can place cds in stage "
          << self.ddm.can_place_cds_in_stage(b_table_to_use, already_placed_b));

    choice = TablePlacement::CALC_STAGE;
    if (a->stage < b->stage) return true;
    if (a->stage > b->stage) return false;

    choice = TablePlacement::PROV_STAGE;
    bool provided_stage = false;
    int a_provided_stage = a->table->get_provided_stage(a->stage);
    int b_provided_stage = b->table->get_provided_stage(b->stage);

    if (a_provided_stage >= 0 && b_provided_stage >= 0) {
        if (a_provided_stage != b_provided_stage)
            return a_provided_stage < b_provided_stage;
        // Both tables need to be in THIS stage...
        provided_stage = true;
    } else if (a_provided_stage >= 0 && b_provided_stage < 0) {
        return true;
    } else if (b_provided_stage >= 0 && a_provided_stage < 0) {
        return false;
    }

    choice = TablePlacement::PRIORITY;
    auto a_priority_str = a->table->get_placement_priority_string();
    auto b_priority_str = b->table->get_placement_priority_string();
    if (a_priority_str.count(b->table->externalName()))
        return true;
    if (b->gw && a_priority_str.count(b->gw->externalName()))
        return true;
    if (b_priority_str.count(a->table->externalName()))
        return false;
    if (a->gw && b_priority_str.count(a->gw->externalName()))
        return false;

    int a_priority = a->table->get_placement_priority_int();
    int b_priority = b->table->get_placement_priority_int();
    if (a_priority > b_priority) return true;
    if (a_priority < b_priority) return false;

    choice = TablePlacement::SHARED_TABLES;
    if (a->complete_shared > b->complete_shared) return true;
    if (a->complete_shared < b->complete_shared) return false;

    ///> Downward Propagation - @sa dynamic_dep_matrix
    choice = TablePlacement::DOWNWARD_PROP_DSC;
    if (down_score.first > down_score.second) return !provided_stage;
    if (down_score.first < down_score.second) return provided_stage;

    ///> Downward Dominance Frontier - for definition,
    ///> see TableDependencyGraph::DepStagesThruDomFrontier
    choice = TablePlacement::DOWNWARD_DOM_FRONTIER;
    if (self.deps.stage_info.at(a_table_to_use).dep_stages_dom_frontier == 0 &&
        self.deps.stage_info.at(b_table_to_use).dep_stages_dom_frontier != 0)
        return true;
    if (self.deps.stage_info.at(a_table_to_use).dep_stages_dom_frontier != 0 &&
        self.deps.stage_info.at(b_table_to_use).dep_stages_dom_frontier == 0)
        return false;

    ///> Direct Dependency Chain without propagation
    int a_local = self.deps.stage_info.at(a_table_to_use).dep_stages_control_anti;
    int b_local = self.deps.stage_info.at(b_table_to_use).dep_stages_control_anti;
    choice = TablePlacement::LOCAL_DSC;
    if (a_local > b_local) return true;
    if (a_local < b_local) return false;


    ///> If the control dominating set is completely placeable
    choice = TablePlacement::CDS_PLACEABLE;
    if (self.ddm.can_place_cds_in_stage(a_table_to_use, already_placed_a) &&
        !self.ddm.can_place_cds_in_stage(b_table_to_use, already_placed_b))
        return true;

    if (self.ddm.can_place_cds_in_stage(b_table_to_use, already_placed_b) &&
        !self.ddm.can_place_cds_in_stage(a_table_to_use, already_placed_a))
        return false;

    ///> If the table needs more match entries.  This can help pack more logical tables earlier
    ///> that have higher stage requirements
    choice = TablePlacement::NEED_MORE;
    if (b->need_more_match && !a->need_more_match) return true;
    if (a->need_more_match && !b->need_more_match) return false;

    if (self.deps.stage_info.at(a_table_to_use).dep_stages_dom_frontier != 0) {
        choice = TablePlacement::CDS_PLACE_COUNT;
        int comp = self.ddm.placeable_cds_count(a_table_to_use, already_placed_a) -
                   self.ddm.placeable_cds_count(b_table_to_use, already_placed_b);
        if (comp != 0)
            return comp > 0;
    }

    ///> Original dependency metric.  Feels like it should be deprecated
    int a_deps_stages = self.deps.stage_info.at(a_table_to_use).dep_stages;
    int b_deps_stages = self.deps.stage_info.at(b_table_to_use).dep_stages;
    choice = TablePlacement::LOCAL_DS;
    if (a_deps_stages > b_deps_stages) return true;
    if (a_deps_stages < b_deps_stages) return false;

    ///> Total dependencies with dominance frontier summed
    int a_dom_frontier_deps = self.ddm.total_deps_of_dom_frontier(a_table_to_use);
    int b_dom_frontier_deps = self.ddm.total_deps_of_dom_frontier(b_table_to_use);
    choice = TablePlacement::DOWNWARD_TD;
    if (a_dom_frontier_deps > b_dom_frontier_deps) return true;
    if (b_dom_frontier_deps > a_dom_frontier_deps) return false;

    ///> Average chain length of all tables within the dominance frontier
    double a_average_cds_deps = self.ddm.average_cds_chain_length(a_table_to_use);
    double b_average_cds_deps = self.ddm.average_cds_chain_length(b_table_to_use);
    choice = TablePlacement::AVERAGE_CDS_CHAIN;
    if (a_average_cds_deps > b_average_cds_deps) return true;
    if (b_average_cds_deps > a_average_cds_deps) return false;


    ///> If the entirety of the control dominating set is placed vs. not
    choice = TablePlacement::NEXT_TABLE_OPEN;
    int a_next_tables_in_use = a_table_to_use == a->gw ? 1 : 0;
    int b_next_tables_in_use = b_table_to_use == b->gw ? 1 : 0;

    int a_dom_set_size = self.ntp.control_dom_set.at(a_table_to_use).size() - 1;
    int b_dom_set_size = self.ntp.control_dom_set.at(b_table_to_use).size() - 1;;

    if (a_dom_set_size <= a_next_tables_in_use && b_dom_set_size > b_next_tables_in_use)
        return true;
    if (a_dom_set_size > a_next_tables_in_use && b_dom_set_size <= b_next_tables_in_use)
        return false;

    ///> Local dependencies
    choice = TablePlacement::LOCAL_TD;
    int a_total_deps = self.deps.happens_before_dependences(a->table).size();
    int b_total_deps = self.deps.happens_before_dependences(b->table).size();
    if (a_total_deps < b_total_deps) return true;
    if (a_total_deps > b_total_deps) return false;

    choice = TablePlacement::DEFAULT;
    return true;
}

class DumpSeqTables {
    const IR::MAU::TableSeq     *seq;
    bitvec                      which;
    friend std::ostream &operator<<(std::ostream &out, const DumpSeqTables &);
 public:
    DumpSeqTables(const IR::MAU::TableSeq *s, bitvec w) : seq(s), which(w) {}
};
std::ostream &operator<<(std::ostream &out, const DumpSeqTables &s) {
    const char *sep = "";
    for (auto i : s.which) {
        if (i >= 0 && (size_t)i < s.seq->tables.size())
            out << sep << s.seq->tables[i]->name;
        else
            out << sep << "<oob " << i << ">";
        sep = " "; }
    return out;
}

std::ostream &operator<<(std::ostream &out, const ordered_set<const IR::MAU::Table *> &s) {
    const char *sep = "";
    for (auto *tbl : s) {
        out << sep << tbl->name;
        sep = ", "; }
    return out;
}

/**
 * In Tofino, a table that is split across multiple stages requires using the next table
 * to advance tables, and thus cannot start anything else on the worklist until that table
 * is fully finished.
 *
 * This does not matter when long branches are turned on, as the next table is no longer the
 * limiting factor.
 */
bool DecidePlacement::can_place_with_partly_placed(const IR::MAU::Table *tbl,
        const ordered_set<const IR::MAU::Table *> &partly_placed,
        const Placed *placed) {
     if (!(Device::numLongBranchTags() == 0 || self.options.disable_long_branch))
         return true;

    for (auto pp : partly_placed) {
        if (pp == tbl || placed->is_match_placed(tbl) || placed->is_match_placed(pp))
            continue;
        if (!self.mutex(pp, tbl) && !self.siaa.mutex_through_ignore(pp, tbl)) {
            LOG3("  - skipping " << tbl->name << " as it is not mutually "
                 "exclusive with partly placed " << pp->name);
            return false;
        }
    }
    return true;
}

/**
 * The purpose of this function is to delay the beginning of placing gateway tables in
 * Tofino2 long before any other their control dependent match tables are placeable.
 * The goal of this is to reduce the number of live long branches at the same time.
 *
 * This is a decent holdover until the algorithm can track long branch usage during table
 * placement and guarantee that no constraints are broken from this angle.
 *
 * Basic algorithm:
 *
 * Let's say I have the following program snippet:
 *
 *     dep_table_1.apply();
 *     dep_table_2.apply();
 *
 *     if (cond-0) {
 *         if (cond-1) {
 *             match_table.apply();
 *         }
 *     }
 *
 * Now cond-0 may be placeable very early due to dependences, but the match_table might be
 * in a much later stage.  The goal is to not start placing cond-0 until all of the logical
 * match dependences are not broken.  The tables that must be placed for match_table to be
 * placed ared dep_table_1, dep_table_2, cond-0 and cond-1.  The checks are if a table is
 * already placed (Check #1) or if a table is control dependent on cond-0 (Check #2).
 *
 * However, I found that this wasn't enough. Examine the following example:
 *
 *     if (cond-0) {
 *         if (cond-1) {
 *             match_table.apply();
 *         }
 *     } else {
 *         if (cond-2) {
 *             match_table.apply();
 *         }
 *     }
 *
 * Now in this example, when trying to place cond-1, the dependencies will notice cond-2 is
 * not placed, and will not start cond-1.  However, when trying to place cond-2, the dependencies
 * will notice cond-1 is not placed either and will not start placing cond-2.  This deadlock
 * is only currently stoppable by Check #3.
 *
 * The eventual goal is to get rid of this check and just verify that the compiler is not
 * running out of long branches.
 */
bool DecidePlacement::gateway_thread_can_start(const IR::MAU::Table *tbl, const Placed *placed) {
    if (!Device::hasLongBranches() || self.options.disable_long_branch)
        return true;
    if (!tbl->uses_gateway())
        return true;
    // The gateway merge constraints are checked in an early function.  This is
    // for unmergeable gateways.
    auto gmc = self.gateway_merge_choices(tbl);
    if (gmc.size() > 0)
        return true;

    int non_cd_gw_tbls = 0;
    bool placeable_table_found = false;
    for (auto cd_tbl : self.ntp.control_dom_set.at(tbl)) {
        if (cd_tbl == tbl) continue;
        if (cd_tbl->uses_gateway()) continue;
        non_cd_gw_tbls++;
        bool any_prev_unaccounted = false;
        for (auto prev : self.deps.happens_logi_after_map.at(cd_tbl)) {
            if (prev->uses_gateway()) continue;   // Check #3 from comments
            if (placed && placed->is_placed(prev)) continue;   // Check #1 from comments
            if (self.ntp.control_dom_set.at(tbl).count(prev)) continue;   // Check #2 from comments
            any_prev_unaccounted = true;
            break;
        }
        if (any_prev_unaccounted) continue;
        placeable_table_found = true;
        break;
    }
    return placeable_table_found || non_cd_gw_tbls == 0;
}

void DecidePlacement::initForPipe(const IR::BFN::Pipe *pipe,
        ordered_set<const GroupPlace *> &work) {
    size_t gress_index = 0;
    for (auto th : pipe->thread) {
        if (th.mau && th.mau->tables.size() > 0) {
            new GroupPlace(*this, work, {}, th.mau);
            self.table_in_gress[gress_index] = true;
        }
        gress_index++;
    }
    if (pipe->ghost_thread.ghost_mau && pipe->ghost_thread.ghost_mau->tables.size() > 0) {
        new GroupPlace(*this, work, {}, pipe->ghost_thread.ghost_mau); }
    self.rejected_placements.clear();
    saved_placements.clear();
    bt_attempts.clear();
}

void DecidePlacement::recomputePartlyPlaced(const Placed *done,
                                            ordered_set<const IR::MAU::Table *> &partly_placed) {
    partly_placed.clear();
    // Also update the starter pistol table from scratch
    self.starter_pistol = { { nullptr, nullptr } };
    for (auto *p = done; p; p = p->prev) {
        if (p->table->created_during_tp) {
            if (p->table->name == "$ingress_starter_pistol")
                self.starter_pistol[INGRESS] = p->table;
            else if (p->table->name == "$egress_starter_pistol")
                self.starter_pistol[EGRESS] = p->table;
            else
                BUG("Unsupported internal table name %s", p->table->name);
        } else if (!done->is_placed(p->table)) {
            partly_placed.insert(p->table);
        }
    }
}

std::optional<DecidePlacement::BacktrackPlacement*>
DecidePlacement::find_previous_placement(const Placed *best, int offset, bool local_bt,
                                         int process_stage) {
    auto &info = saved_placements.at(best->name);
    auto &bt = local_bt ? info.last_pass : info.early;
    std::optional<DecidePlacement::BacktrackPlacement*> rv = std::nullopt;
    int best_init_stage = best->init_stage();
    for (auto it = bt.rbegin(); it != bt.rend(); ++it) {
        int stage = it->first;
        DecidePlacement::BacktrackPlacement& placement = it->second;
        int plac_init_stage = placement.get_placed()->init_stage();
        // Tried placement are not a good option
        if (placement.is_best_placement() || placement.tried) {
            continue;
        } else if (((stage + offset) <= process_stage) && !placement->need_more) {
            if (bt_attempts.count(best->name)) {
                if (bt_attempts.at(best->name).count(stage)) {
                    LOG3("Found other placement for table:" << best->name << " at stage:" <<
                         stage << " but a variant of it was already tried");
                    rv = &placement;
                    continue;
                }
            }
            LOG3("Found other placement for table:" << best->name << " at stage:" << stage);
            bt_attempts[best->name].insert(stage);
            return &placement;
        } else if ((plac_init_stage + offset) <= best_init_stage &&
                   (stage + offset) < process_stage) {
            if (bt_attempts.count(best->name)) {
                if (bt_attempts.at(best->name).count(stage)) {
                    LOG3("Found other placement for split table:" << best->name << " at stage:" <<
                         stage << " but a variant of it was already tried");
                    rv = &placement;
                    continue;
                }
            }
            LOG3("Found other initial placement for split table:" << best->name <<
                 " at stage:" << stage);
            bt_attempts[best->name].insert(stage);
            return &placement;
        }
    }
    return rv;
}

std::optional<DecidePlacement::BacktrackPlacement*>
DecidePlacement::find_backtrack_point(const Placed *best, int offset, bool local_bt) {
    std::optional<DecidePlacement::BacktrackPlacement*> cc = std::nullopt;
    ordered_set<const IR::MAU::Table*> to_be_analyzed;
    std::map<const IR::MAU::Table*, const Placed*> same_stage;
    int process_stage = best->stage;
    const Placed *last_stage = best;

    to_be_analyzed.insert(best->table);
    ordered_set<const IR::MAU::Table*> to_be_analyzed_next_pre;

    LOG3("Find " << (local_bt ? "local" : "global") << " backtracking point for table " <<
         best->name << " and offset " << offset);

    if (saved_placements.count(best->name)) {
        auto bt = find_previous_placement(best, offset, local_bt, process_stage);
        if (bt)
            return bt;
    }

    LOG3("Initial process_stage:" << process_stage);
    while (last_stage) {
        if (last_stage->stage < process_stage) {
            process_stage = last_stage->stage;
            LOG3("Initial updated process_stage:" << process_stage);
            break;
        }
        same_stage.insert({ last_stage->table, last_stage });
        if (last_stage->gw)
            same_stage.insert({ last_stage->gw, last_stage });
        last_stage = last_stage->prev;
    }

    /* Revisit the already placed tables that can have an impact on the actual fitting problem
     * by looking at the control and data dependency. The control and data dependency analysis
     * start with the initial table that failed the downward dependency compute. Upward dependency
     * tables are added to the pool when analyzing the actual placed table. Each tables that are
     * found to be critical for the actual fitting issue are analyzed to see if other placement
     * are available that would relaxe the actual constraint.
     */
    while (!to_be_analyzed.empty()) {
        ordered_set<const IR::MAU::Table*> to_be_analyzed_next_post;
        ordered_set<const IR::MAU::Table*> potential_container_conflict;
        for (const IR::MAU::Table* t1 : to_be_analyzed) {
            // Data dependency
            ordered_set<const IR::MAU::Table*> prev = self.deps.happens_phys_after_map.at(t1);
            to_be_analyzed_next_pre.insert(prev.begin(), prev.end());
            to_be_analyzed_next_pre.insert(t1);

            // Control dependency
            ordered_set<const IR::MAU::Table*> log_prev = self.deps.happens_logi_after_map.at(t1);
            for (const IR::MAU::Table* log_tbl : log_prev) {
                // If the analyzed table is control dependent on another table located in the same
                // stage then both table must be moved to an earlier stage.
                if (same_stage.count(log_tbl)) {
                    const Placed *tbl_pl = same_stage.at(log_tbl);
                    if (saved_placements.count(tbl_pl->name)) {
                        auto bt = find_previous_placement(tbl_pl, offset, local_bt,
                                                          process_stage + 1);
                        if (bt)
                            return bt;
                    }
                }
                // Data dependency of the control dependent table must also be analyzed.
                if (!to_be_analyzed_next_pre.count(log_tbl)) {
                    ordered_set<const IR::MAU::Table*> phys_prev =
                                                       self.deps.happens_phys_after_map.at(log_tbl);
                    to_be_analyzed_next_pre.insert(phys_prev.begin(), phys_prev.end());
                }
            }
        }
        // Do not analyse stage 0 since we can't move these table in a previous stage.
        if (!process_stage)
            return std::nullopt;

        // It is possible that a table can't be placed into a given stage because of container
        // conflict with another table in the same stage.
        for (const IR::MAU::Table* t1 : to_be_analyzed) {
            if (self.deps.container_conflicts.count(t1)) {
                potential_container_conflict.insert(self.deps.container_conflicts.at(t1).begin(),
                                                    self.deps.container_conflicts.at(t1).end());
            }
        }

        same_stage.clear();
        while (last_stage) {
            if (last_stage->stage < process_stage) {
                process_stage = last_stage->stage;
                LOG3("Updated process_stage:" << process_stage);
                break;
            }
            if (to_be_analyzed_next_pre.count(last_stage->table) ||
                (last_stage->gw && to_be_analyzed_next_pre.count(last_stage->gw))) {
                LOG3("Found dependant table:" << last_stage->name << " in previous stage:" <<
                     process_stage);
                to_be_analyzed_next_post.insert(last_stage->table);
                if (saved_placements.count(last_stage->name)) {
                    auto bt = find_previous_placement(last_stage, offset, local_bt, process_stage);
                    if (bt)
                        return bt;
                }
            } else if (!cc && !self.ignoreContainerConflicts &&
                       (potential_container_conflict.count(last_stage->table) ||
                       (last_stage->gw && potential_container_conflict.count(last_stage->gw)))) {
                LOG3("Found table with potential container conflict:" << last_stage->name <<
                     " in previous stage:" << process_stage);
                if (saved_placements.count(last_stage->name)) {
                    cc = find_previous_placement(last_stage, offset, local_bt, process_stage);
                    if (cc)
                        LOG3("Caching potential container conflict table in the search for a " <<
                             "direct dependent table");
                }
            }

            same_stage.insert({ last_stage->table, last_stage });
            if (last_stage->gw)
                same_stage.insert({ last_stage->gw, last_stage });
            last_stage = last_stage->prev;
        }
        // No direct dependent table found, go for the one with container conflict
        if (cc)
            return cc;

        if (to_be_analyzed_next_post.empty() && process_stage && last_stage)
            LOG3("Analyzing previous stage with the same dependent table");
        else
            to_be_analyzed = to_be_analyzed_next_post;

        if (!to_be_analyzed.empty())
            LOG3("Analyzing previous stage next");
    }

    return std::nullopt;
}

#ifdef MULTITHREAD
/* This class is used to evaluate multiple possible table placement in parallel using x number
 * of worker threads. Typically the table placement code evaluate all of the table that can be
 * placed at any given time and choose which one to select based on some heuristics. The evaluated
 * table that are not selected can be saved as backtrack point if table placement want to try a
 * different approach going forward. The evaluation does not update any IR element nor update
 * any storage class so it is a good candidate for parallel execution. The table evaluation
 * are distributed amond the worker thread in any given order but each request have an ID that
 * specify the original request sorting order. This ID is used to re-order the table evaluation
 * with the exact same order as initially requested. The idea is to have the exact same result
 * with or without parallel evaluation.
 *
 * The multithreading code can be enabled by defining "ENABLE_MULTITHREAD",
 * e.g. "-DENABLE_MULTITHREAD=ON" when calling the "bootstrap_bfn_compilers.sh" step. This variable
 * is also used in the P4C Frontend to make sure logging is thread safe.
 *
 * Enabling the multithreading code also require the garbage collector to be compiled with thread
 * support. We experimented it on GC 8.2.0 using the settings:
 * "--enable-large-config --enable-cplusplus --enable-shared --enable-threads=posix"
 *
 * Unfortunately the performance result was not as good as we would have expected. The main issue
 * is related to the fact that memory allocation/deallocation is consuming most of the CPU cycles
 * and the garbage collector serialize all of these call. The result is that most of the threads
 * are waiting on a lock at any given time. This also increase the scheduler overhead that have
 * to always swap from one thread to another the control of the GC lock. Performance test on large
 * profile show a performance reduction of 0 to 20% when using multithreading vs single thread.
 *
 * Enabling multithreading also scramble the logging because this part was not being thought with
 * parallel execution. We decided to keep the multithreading code in table allocation if we
 * found a solution for the garbage collector and logging going forward.
 */
class DecidePlacement::TryPlacedPool {
    const DecidePlacement &self;
    std::vector<pthread_t> workers;
    std::atomic<bool> terminated{false};
    std::condition_variable_any queue_CV;
    std::mutex queue_mutex;

    // Carry all the information needed to evaluate a table.
    struct request_arg {
        const IR::MAU::Table *t;
        const Placed *done;
        const StageUseEstimate &current;
        TablePlacement::GatewayMergeChoices *gmc;
        const GroupPlace *group;

        request_arg(const IR::MAU::Table *t, const Placed *done, const StageUseEstimate &current,
                    TablePlacement::GatewayMergeChoices *gmc, const GroupPlace *group) :
                        t(t), done(done), current(current), gmc(gmc), group(group) { }
    };
    std::queue<std::pair<int, struct request_arg*>> work_queue;
    std::condition_variable_any res_CV;
    std::mutex res_mutex;
    std::map<int, std::pair<safe_vector<TablePlacement::Placed *>, const GroupPlace *>> work_result;

    int num_req = 0;
    int exe_req = 0;

    // Required to bind a pthread to an object
    static void* workerWaitCast(void* arg) {
        DecidePlacement::TryPlacedPool* tp = reinterpret_cast<DecidePlacement::TryPlacedPool*>(arg);
        tp->workerWait();
        return NULL;
    }
    void* workerWait();

 public:
    explicit TryPlacedPool(DecidePlacement &self, int n) : self(self) {
        static bool init_mt = true;
        if (init_mt) {
            GC_allow_register_threads();
            init_mt = false;
        }
        for (int i = 0; i < n; i++) {
            pthread_t tid;
            pthread_attr_t attr;
            int err;
            // This value is to make sure the created stack will not be cached
            size_t stack_size = 1024 * 1024 * 64;  // 64MB
            err = pthread_attr_init(&attr);
            BUG_CHECK(!err, "Pthread Attribute initialization fail with error: %d", err);
            err = pthread_attr_setstacksize(&attr, stack_size);
            BUG_CHECK(!err, "Pthread Attribute Set Stack Size fail with error: %d", err);
            err = pthread_create(&tid, &attr, workerWaitCast, this);
            BUG_CHECK(!err, "Pthread Creation fail with error: %d", err);
            err = pthread_attr_destroy(&attr);
            BUG_CHECK(!err, "Pthread Attribute destroy fail with error: %d", err);
            workers.push_back(tid);
        }
    }
    ~TryPlacedPool() {
        if (!terminated.load()) {
            shutdown();
        }
    }
    void cleanup();
    void shutdown();
    void addReq(const IR::MAU::Table *t, const Placed *done, const StageUseEstimate &current,
                const TablePlacement::GatewayMergeChoices &gmc, const GroupPlace *group);
    bool fillTrial(safe_vector<const Placed *> &trial, bitvec &trial_tables);
};

// Worker thread that process request until the "terminated" flag is set
void* DecidePlacement::TryPlacedPool::workerWait() {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);

    while (!terminated.load()) {
        std::pair<int, struct request_arg*> req;
        {
            std::unique_lock<std::mutex> guard(queue_mutex);
            queue_CV.wait(guard, [&]{ return !work_queue.empty() || terminated.load(); });
            if (terminated.load()) {
                break;
            }
            req = work_queue.front();
            work_queue.pop();
        }
        safe_vector<TablePlacement::Placed *> res = self.self.try_place_table(req.second->t,
                                                                              req.second->done,
                                                                              req.second->current,
                                                                              *req.second->gmc);
        {
            std::lock_guard<std::mutex> guard(res_mutex);
            work_result[req.first] = std::make_pair(std::move(res), req.second->group);
            exe_req++;
            res_CV.notify_one();
        }
    }
    GC_unregister_my_thread();
    return NULL;
}

// Add a request to be processed by one of the Worker Thread
void DecidePlacement::TryPlacedPool::addReq(const IR::MAU::Table *t, const Placed *done,
                                            const StageUseEstimate &current,
                                            const TablePlacement::GatewayMergeChoices &gmc,
                                            const GroupPlace *group) {
    TablePlacement::GatewayMergeChoices* gmc_copy = new TablePlacement::GatewayMergeChoices(gmc);
    request_arg *req_arg = new request_arg(t, done, current, gmc_copy, group);
    std::lock_guard<std::mutex> guard(queue_mutex);
    work_queue.push(std::make_pair(num_req++, req_arg));
    queue_CV.notify_one();
}

// Wait for the worker thread to finalize all the requested evaluation than fill the trial vector
// for further analysis by heuristic based best choice
bool DecidePlacement::TryPlacedPool::fillTrial(safe_vector<const Placed *> &trial,
                                               bitvec &trial_tables) {
    int expected_req;
    {
        std::lock_guard<std::mutex> guard(queue_mutex);
        expected_req = num_req;
    }
    {
        std::unique_lock<std::mutex> guard(res_mutex);
        res_CV.wait(guard, [&]{ return exe_req == expected_req; });
    }
    for (auto &res : work_result) {
        for (auto &pl : res.second.first) {
            if (trial_tables[self.self.uid(pl->table)])
                continue;

            pl->group = res.second.second;
            trial.push_back(pl);
            trial_tables.setbit(self.self.uid(pl->table));
        }
    }
    return true;
}

// Reset the executed count to zero for the next round of evaluation
void DecidePlacement::TryPlacedPool::cleanup() {
    {
        std::lock_guard<std::mutex> guard(queue_mutex);
        BUG_CHECK(work_queue.empty(), "Multi-Threaded Work Queue not entirely processed");
        num_req = 0;
    }
    {
        std::lock_guard<std::mutex> guard(res_mutex);
        work_result.clear();
        exe_req = 0;
    }
}

// Shutdown all the worker threads
void DecidePlacement::TryPlacedPool::shutdown() {
    void *status;
    terminated = true;
    // Wake up the waiting threads
    queue_CV.notify_all();
    for (pthread_t &tid : workers) {
        int err = pthread_join(tid, &status);
        BUG_CHECK(!err, "Pthread Join fail with error: %d", err);
    }
}
#endif

/* This class encapsulate most of the high level backtracking services. Backtracking is used
 * mainly for:
 * 1 - Find an alternative solution to workaround a dependency problem
 * 2 - Build multiple solution for a single stage to evaluate them after and choose the best one
 *     (resource based allocation)
 * 3 - Build multiple entire placement and compare them if none of them fit in the actual device
 *     resources. We currently have 4 strategies
 *     a) Dependency Only
 *     b) Dependency Only with backtracking
 *     c) Resource based allocation
 *     d) Resource based allocation with backtracking
 */
class DecidePlacement::BacktrackManagement {
    DecidePlacement &self;

    ResourceBasedAlloc res_based_alloc;  // Handle resource based allocation
    FinalPlacement final_placement;      // Carry the complete solution based on allocation strategy

    // These references are used to update local element of default_table_placement()
    ordered_set<const GroupPlace *> &active_work;
    ordered_set<const IR::MAU::Table *> &partly_placed;
    const Placed *&active_placed;
    Backfill &backfill;

    // We only move from dependency only strategy to resource based allocation if this flag is set
    bool ena_resource_mode = false;

    // Track the first table allocated as a backtrack point to create multiple complete solutions
    BacktrackPlacement *start_flow = nullptr;

    std::list<BacktrackPlacement *> initial_stage_options;

 public:
    BacktrackManagement(DecidePlacement &self, ordered_set<const GroupPlace *> &w,
                        ordered_set<const IR::MAU::Table *> &p, const Placed *&a, Backfill &b) :
                        self(self), res_based_alloc(self, w, p, a), final_placement(self, w, p, a),
                        active_work(w), partly_placed(p), active_placed(a), backfill(b) {
        self.backtrack_count = 0;
        self.resource_mode = false;
        // This is for the table first approach
        if (BFNContext::get().options().alt_phv_alloc) {
            switch (self.self.summary.getActualState()) {
                case State::ALT_INITIAL:
                    // first round of table placement does not enable resource-based alloc.
                    self.MaxBacktracksPerPipe = 32;
                    break;
                case State::ALT_RETRY_ENHANCED_TP:
                    // retry table placement with resource-based allocation and backtracking ON.
                    ena_resource_mode = true;
                    self.resource_mode = true;
                    self.MaxBacktracksPerPipe = 64;
                    break;
                case State::ALT_FINALIZE_TABLE_SAME_ORDER:
                case State::ALT_FINALIZE_TABLE_SAME_ORDER_TABLE_FIXED:
                    self.MaxBacktracksPerPipe = -1;
                    break;
                case State::ALT_FINALIZE_TABLE:
                    // final round, enable both resource-based allocation and backtracking.
                    ena_resource_mode = true;
                    self.MaxBacktracksPerPipe = 64;
                    break;
                default:
                    self.MaxBacktracksPerPipe = -1;
                    break;
            }
            return;
        }

        // This is to balance compile time with benefit from backtracking
        switch (self.self.summary.getActualState()) {
            case State::INITIAL:
                // Disable backtracking on initial state.
                self.MaxBacktracksPerPipe = -1;
                break;
            case State::NOCC_TRY1:
            case State::NOCC_TRY2:
                ena_resource_mode = true;
                self.MaxBacktracksPerPipe = 12;
                break;
            case State::REDO_PHV1:
            case State::REDO_PHV2:
                ena_resource_mode = true;
                self.MaxBacktracksPerPipe = 32;
                break;
            default:
                self.MaxBacktracksPerPipe = -1;
                break;
        }
    }

    // Return an attached memory pointer if the actual placement state is invalid. Invalid placement
    // mean that a shared attach table was placed on a stage before all of the match table that
    // refer to that memory are placed. This is only a problem if we are moving from one stage to
    // the next and the previous stage was still invalid.
    std::optional<const IR::MAU::AttachedMemory*> is_unstable_placement(const Placed *placed) {
        for (const Placed *p = placed->prev; p; p = p->prev) {
            for (auto *ba : p->table->attached) {
                if (ba->attached->direct) continue;
                if (self.self.can_duplicate(ba->attached)) continue;
                if (p->attached_entries.at(ba->attached).entries != 0 &&
                    self.self.attached_to.count(ba->attached)) {
                    for (auto &tbl : self.self.attached_to.at(ba->attached)) {
                        if (!placed->is_match_placed(tbl))
                            return ba->attached;
                    }
                }
            }
        }
        return std::nullopt;
    }

    // Save future backtrack position and handle resouce based allocation solution buildup. Return
    // true if a backtracking position was found, false otherwise.
    bool update_bt_point(const Placed *best, const safe_vector<const Placed *> &trial) {
        // Always try to respect the stage and placement pragma even when backtracking.
        bool best_with_pragmas = false;
        if (best->table->get_provided_stage() >= 0 ||
            best->table->get_placement_priority_int() > 0) {
            LOG3("Best table have stage or priority pragma");
            best_with_pragmas = true;
        } else {
            std::set<cstring> pri_tbls = best->table->get_placement_priority_string();
            for (auto t : trial) {
                if (pri_tbls.count(t->table->externalName())) {
                    LOG3("Best table have priority pragma with tbl " << t->name);
                    best_with_pragmas = true;
                    break;
                }
            }
        }

        // Apply that on stage transition
        if (best->prev && best->prev->stage != best->stage) {
            std::optional<const IR::MAU::AttachedMemory*> att = is_unstable_placement(best->prev);
            // Stage was completed and the placement was invalid. We need to fix it or return an
            // error since the behaviour will be incorrect.
            if (att) {
                std::string tbls_name;
                for (auto &tbl : self.self.attached_to.at(*att)) {
                    if (!tbls_name.empty())
                        tbls_name += ", ";
                    tbls_name += tbl->externalName();
                    self.self.not_eligible.insert(tbl);
                }
                self.error("Table placement was not able to allocate %s in the same "
                           "stage along with %s", tbls_name, (*att)->toString());
                // Try to fix it through backtracking by making sure none of these match table
                // are assigned on this stage.
                initial_stage_options.remove_if([&](auto btp){
                    return self.self.not_eligible.count(btp->get_placed()->table);});
                if (!initial_stage_options.empty()) {
                    LOG3("Try to fix unstable placement through backtracking");
                    backtrack_to(initial_stage_options.front());
                    initial_stage_options.pop_front();
                    return true;
                }
            // Resource mode initial backtracking point are set inside of select_best_solution()
            } else if (!self.resource_mode) {
                initial_stage_options.clear();
                self.self.not_eligible.clear();
                for (auto t : trial) {
                    if (t->stage == best->stage) {
                        LOG3("Adding table:" << t->name << " in the initial stage option");
                        auto btp = new BacktrackPlacement(self, t, active_work, false);
                        initial_stage_options.push_back(btp);
                    }
                }
            }
        }

        // Resource mode handling
        if (self.resource_mode && active_placed && best->stage > active_placed->stage) {
            if (!is_unstable_placement(best))
                self.savePlacement(best, active_work, true);
            LOG3("Resource mode and placement completed for stage:" << active_placed->stage);
            self.self.not_eligible.clear();
            PlacementScore *pl_score = res_based_alloc.add_stage_complete(best, active_work, trial,
                                                                          best_with_pragmas);
            if (res_based_alloc.found_other_placement()) {
                LOG3("Found incompleted placement to try");
                backfill.clear();
                return true;
            } else {
                LOG3("Found NO other incomplete placement");
                bool cur_is_best = res_based_alloc.select_best_solution(best, pl_score,
                                                                        initial_stage_options);
                backfill.clear();
                if (!cur_is_best)
                    return true;
            }
        }

        // Tables being placed because of pragmas can't be overriden by backtrack. Other table
        // possible placement are saved under some scenario to be re-used in future backtracking
        // situation.
        if (!best_with_pragmas) {
            for (auto t : trial) {
                if (t->stage == best->stage) {
                    if (self.resource_mode)
                        res_based_alloc.add_placed_pos(t, active_work, t == best);

                    if (!is_unstable_placement(t))
                        self.savePlacement(t, active_work, t == best);
                }
            }
        } else {
            if (self.resource_mode)
                res_based_alloc.add_placed_pos(best, active_work, true);

            if (!is_unstable_placement(best))
                self.savePlacement(best, active_work, true);
        }

        // Save the first placement position we encounter to use it as baseline for all complete
        // solutions.
        if (!start_flow) {
            start_flow = new BacktrackPlacement(self, best, active_work, true);
            for (auto t : trial) {
                if (t->stage == best->stage) {
                    LOG3("Adding table:" << t->name << " in the initial stage option");
                    auto btp = new BacktrackPlacement(self, t, active_work, false);
                    initial_stage_options.push_back(btp);
                }
            }
        }

        return false;
    }

    // Backtrack to this placement
    void backtrack_to(BacktrackPlacement *bt) {
        active_placed = bt->reset_place_table(active_work);
        self.recomputePartlyPlaced(active_placed, partly_placed);
        backfill.clear();
        res_based_alloc.clear_stage();
    }

    // Try to find a backtrack placement that relaxe the actual constraint if possible. Change
    // strategy from dependency only to resource based if the number of backtracking allowed was
    // exceeded. Return true if a backtrack placement to try was found, false otherwise.
    bool find_backtrack_solution(const Placed *best, int dep_chain) {
        // Incomplete placement before trying with backtracking.
        if (self.backtrack_count == 0) {
            BacktrackPlacement *bt = new BacktrackPlacement(self, best, active_work, true);
            final_placement.add_incomplete_placement(bt);
        }

        LOG3("Found dependency chain of " << dep_chain << " at stage " << best->stage);
        LOG3("Actual backtrack count:" << self.backtrack_count << " with max backtrack count:"
             << self.MaxBacktracksPerPipe);

        // Try to increase the solution by doing "local" backtracking which mean that we
        // prefer latest backtracking point for a given table. If this strategy fail, we
        // move to the "global" backtracking point which can go back anywhere in the past.
        if (self.backtrack_count < self.MaxBacktracksPerPipe) {
            int offset = best->stage + dep_chain - (Device::numStages() - 1);
            std::optional<BacktrackPlacement*> bt = self.find_backtrack_point(best, offset, true);
            if (bt) {
                LOG3("Found local backtracking point");
                backtrack_to(*bt);
                return true;
            } else {
                std::optional<BacktrackPlacement*> bt = self.find_backtrack_point(best, offset,
                                                                                    false);
                if (bt) {
                    LOG3("Found global backtracking point");
                    backtrack_to(*bt);
                    return true;
                }
            }
        }

        if (start_flow && !self.resource_mode && ena_resource_mode) {
            // Non resource mode with backtracking incomplete placement
            if (self.backtrack_count) {
                BacktrackPlacement *bt = new BacktrackPlacement(self, best, active_work, true);
                final_placement.add_incomplete_placement(bt);
            }
            // Restart table placement from the beginning in resource mode
            LOG3("Try with resource mode enabled");
            backtrack_to(start_flow);
            self.saved_placements.clear();
            self.bt_attempts.clear();
            self.backtrack_count = 0;
            self.resource_mode = true;
            return true;
        } else {
            // Set the backtrack_count to the maximum value to disable any more
            // backtracking for this table allocation pass.
            self.backtrack_count = self.MaxBacktracksPerPipe + 1;
        }
        return false;
    }

    // Add a complete solution for one of the strategy
    void add_complete_solution() {
        BacktrackPlacement *bt = new BacktrackPlacement(self, active_placed, active_work, true);
        final_placement.add_complete_placement(bt);
    }

    // Select the best solution if none of them fill all the device requirement
    void select_best_final_solution() {
        final_placement.select_best_final_placement();
    }
};

const DecidePlacement::Placed*
DecidePlacement::default_table_placement(const IR::BFN::Pipe *pipe) {
    LOG1("Table placement starting on " << pipe->canon_name()
         << " with DEFAULT PLACEMENT approach");

    LOG3(TableTree("ingress"_cs, pipe->thread[INGRESS].mau) <<
         TableTree("egress"_cs, pipe->thread[EGRESS].mau) <<
         TableTree("ghost"_cs, pipe->ghost_thread.ghost_mau) );

    ordered_set<const GroupPlace *>     work;  // queue with random-access lookup
    const Placed *placed = nullptr;

    /* all the state for a partial table placement is stored in the work
     * set and placed list, which are const pointers, so we can backtrack
     * by just saving a snapshot of a work set and corresponding placed
     * list and restoring that point */
    initForPipe(pipe, work);

    ordered_set<const IR::MAU::Table *> partly_placed;
    Backfill backfill(*this);
    BacktrackManagement bt_mgmt(*this, work, partly_placed, placed, backfill);
#ifdef MULTITHREAD
    TryPlacedPool placed_pool(*this, jobs);
#endif
    while (true) {
        // Empty work means that all the tables are actually placed. Save it as a complete
        // placement for future comparison.
        if (work.empty()) {
            bt_mgmt.add_complete_solution();
            // No other incomplete placement to finalize
            if (work.empty())
                break;
        }
        erase_if(partly_placed, [placed](const IR::MAU::Table *t) -> bool {
                                        return placed->is_placed(t); });
        if (placed) backfill.set_stage(placed->stage);
        StageUseEstimate current = get_current_stage_use(placed);
        LOG3("stage " << (placed ? placed->stage : 0) << ", work: " << work <<
             ", partly placed " << partly_placed.size() << ", placed " << count(placed) <<
             Log::endl << "    " << current);
        if (!partly_placed.empty())
            LOG5("    partly_placed: " << partly_placed);
        safe_vector<const Placed *> trial;
        bitvec  trial_tables;
#ifdef MULTITHREAD
        placed_pool.cleanup();
#endif
        for (auto it = work.begin(); it != work.end();) {
            // DANGER -- we iterate over the work queue while possibly removing and
            // appending groups.  So care is required to not invalidate the iterator
            // and not miss newly added groups.
            auto grp = *it;
            LOG4("  group " << grp->seq->clone_id << " depth=" << grp->depth);
            if (placed && placed->placed.contains(grp->info.tables)) {
                LOG4("    group " << grp->seq->clone_id << " is now complete");
                LOG5("      removing " << (*it)->seq->clone_id << " from work list (b)");
                it = work.erase(it);
                auto add = grp->finish(work);
                if (it == work.end()) it = add;
                continue; }
            BUG_CHECK(grp->ancestors.count(grp) == 0, "group is its own ancestor!");
            if (Device::numLongBranchTags() == 0 || self.options.disable_long_branch) {
                // Table run only with next_table, so can't continue placing ancestors until
                // this group is finished
                if (LOGGING(5)) {
                    for (auto *s : grp->ancestors)
                        if (work.count(s))
                            LOG5("    removing " << s->seq->clone_id << " from work as it is an " <<
                                 "ancestor of " << grp->seq->clone_id); }
                work -= grp->ancestors; }
            int idx = -1;
            bool done = true;
            bitvec seq_placed;

            bool first_not_yet_placed = true;
            for (auto t : grp->seq->tables) {
                ++idx;
                if (placed && placed->is_placed(t)) {
                    seq_placed[idx] = true;
                    LOG3("    - skipping " << t->name << " as its already done");
                    continue; }
                auto &info = self.tblInfo.at(t);
                if (trial_tables[info.uid])
                    continue;
                if (info.parents && (!placed || (info.parents - placed->match_placed))) {
                    LOG3("    - skipping " << t->name << " as a parent is not yet placed");
                    continue; }

                if (self.options.table_placement_in_order) {
                    if (first_not_yet_placed)
                        first_not_yet_placed = false;
                    else
                        break; }

                bool should_skip = false;  // flag to continue; outer loop;
                for (auto& grp_tbl : grp->seq->tables) {
                    if (self.deps.happens_before_control(t, grp_tbl) &&
                        (!placed || !(placed->is_placed(grp_tbl)))) {
                        LOG3("  - skipping " << t->name << " due to in-sequence control" <<
                            " dependence on " << grp_tbl->name);
                        done = false;
                        should_skip = true;
                        break; } }
                if (should_skip) continue;

                if (!are_metadata_deps_satisfied(placed, t)) {
                    LOG3("  - skipping " << t->name << " because metadata deps not satisfied");
                    // In theory, could continue, but the analysis at this point would be
                    // incorrect
                    done = false;
                    continue;
                }
                for (auto *prev : self.deps.happens_logi_after_map.at(t)) {
                    if (!placed || !placed->is_match_placed(prev)) {
                        LOG3("  - skipping " << t->name << " because it depends on " << prev->name);
                        done = false;
                        should_skip = true;
                        break; } }

                if (!can_place_with_partly_placed(t, partly_placed, placed)) {
                     done = false;
                     continue;
                }

                // Find potential tables this table can be merged with (if it's a gateway)
                TablePlacement::GatewayMergeChoices gmc = self.gateway_merge_choices(t);
                // Prune these choices according to happens after
                std::vector<const IR::MAU::Table*> to_erase;
                for (auto mc : gmc) {
                    // Iterate through all of this merge choice's happens afters and make sure
                    // they're placed
                    for (auto* prev : self.deps.happens_logi_after_map.at(mc.first)) {
                        if (prev == t)
                            continue;
                        if (!placed || !placed->is_placed(prev)) {
                            LOG3("    - removing " << mc.first->name << " from merge list because "
                                 "it depends on " << prev->name);
                            to_erase.push_back(mc.first);
                            break;
                        }
                    }

                    if (!can_place_with_partly_placed(mc.first, partly_placed, placed)) {
                        to_erase.push_back(mc.first);
                    }
                }
                // If we did have choices to merge but all of them are not ready yet, don't try to
                // place this gateway
                if (gmc.size() > 0 && gmc.size() == to_erase.size()) {
                    LOG2("    - skipping gateway " << t->name <<
                         " until mergeable tables are available");
                    should_skip = true;
                    done = false;
                }
                // Finally, erase these choices from gmc
                for (auto mc_unready : to_erase)
                    gmc.erase(mc_unready);

                if (!gateway_thread_can_start(t, placed)) {
                    LOG2("    - skipping gateway " << t->name <<
                         " until any of the control dominating tables can be placed");
                    should_skip = true;
                    done = false;
                }

                // Now skip attempting to place this table if this flag was set at all
                if (should_skip) continue;
#ifdef MULTITHREAD
                placed_pool.addReq(t, placed, current, gmc, grp);
                done = false;
#else
                // Attempt to actually place the table
                auto pl_vec = self.try_place_table(t, placed, current, gmc);
                LOG3("    Pl vector: " << pl_vec);
                done = false;
                for (auto pl : pl_vec) {
                    pl->group = grp;
                    trial.push_back(pl);
                    trial_tables.setbit(self.uid(pl->table));
                }
#endif
            }
            if (done) {
                BUG_CHECK(!placed->is_fully_placed(grp->seq), "Can't find a table to place");
                LOG5("      removing " << (*it)->seq->clone_id << " from work list (c)");
                it = work.erase(it);
            } else {
                it++;
            }
        }
#ifdef MULTITHREAD
        placed_pool.fillTrial(trial, trial_tables);
#endif
        if (work.empty()) continue;
        if (trial.empty()) {
            if (errorCount() == 0) {
                error("Table placement cannot make any more progress.  Though some tables have "
                      "not yet been placed, dependency analysis has found that no more tables are "
                      "placeable.%1%", partly_placed.empty() ? "" :
                      "  This may be due to shared attachments on partly placed tables; may be "
                      "able to avoid the problem with @stage on those tables");
                for (auto *tbl : partly_placed)
                    error("partly placed: %s", tbl); }
            LOG2("table placement ending with unplaced tables:");
            for (auto *info : self.tblByUid) {
                if (placed && placed->placed[info->uid]) continue;
                bool pp = partly_placed.count(info->table);
                bool mp = placed && placed->match_placed[info->uid];
                LOG2("  " << info->table->name  << (pp ? "(pp)" : "") << (mp ? "(mp)" : ""));
            }
            work.clear();
            continue;
        }
        LOG2("found " << trial.size() << " tables that could be placed: " << trial);
        const Placed *best = 0;

        TablePlacement::choice_t choice = TablePlacement::DEFAULT;
        for (auto t : trial) {
            if (best)
                LOG3("For trial t : " << t->name << " with best: " << best->name);
            if (!best) {
                LOG3("Initial best is first table seen: " << t->name);
                best = t;
            } else if (is_better(t, best, choice)) {
                LOG3("    Updating best to " << t->name << " from " << best->name <<
                     " for reason: " << choice);
                self.reject_placement(best, choice, t);
                best = t;
            } else {
                LOG3("    Keeping best " << best->name << " for reason: " << choice);
                self.reject_placement(t, choice, best); } }

        if (bt_mgmt.update_bt_point(best, trial))
            continue;

        if (placed && best->stage > placed->stage &&
            !self.options.disable_table_placement_backfill) {
            const Placed *backfilled = nullptr;
            /* look for a table that could be backfilled */
            for (auto &bf : backfill) {
                if (placed->is_placed(bf.table)) continue;
                if (partly_placed.count(bf.table)) continue;
                if ((backfilled = try_backfill_table(placed, bf.table, bf.before))) {
                    BUG_CHECK(backfilled->is_placed(bf.table), "backfill !is_placed abort");
                    /* Found one -- currently we don't priorities if mulitple tables could
                     * be backfilled; just backfill the first found */
                    break; } }
            if (backfilled) {
                placed = backfilled;
                /* backfilling a table -- abort the current placement and go back and do it
                 * again in case something changed.  It seems that nothing ever should change
                 * so we'll end up finding the same table to place nextyt into the next stage
                 * anyways, but to ocntinue here we'd have to fix up the Placed to refer to
                 * the backfilled placed, and *best is (currently) const.  We also don't look
                 * for newly backfillable tables as that would pretty much require backtracking
                 * and redoing everything in the stage after tha backfilled table (would require
                 * checkpointing and restoring the work list) */
                continue; } }

        if (placed && best->stage != placed->stage) {
            LOG_FEATURE("stage_advance", 2,
                        "Stage " << placed->stage << IndentCtl::indent << Log::endl <<
                        StageSummary(placed->stage, best) << IndentCtl::unindent);
            for (auto t : trial) {
                if (t->stage == best->stage)
                    LOG_FEATURE("stage_advance", 2, "can't place " << t->name << " in stage " <<
                                (t->stage-1) << " : " << t->stage_advance_log); }
            if (self.options.table_placement_long_branch_backtrack &&
                Device::hasLongBranches() && !self.options.disable_long_branch) {
                // check to see if too many long branch tags are needed
                BacktrackPlacement *bt = nullptr;
                int need = longBranchTagsNeeded(placed, work, &bt);
                LOG3("need " << need << " long branches over stage " << placed->stage);
                if (need <= Device::numLongBranchTags() + placed->logical_ids_left()) {
                    if (need > Device::numLongBranchTags())
                        LOG3("  - " << placed->logical_ids_left() << " logical ids left");
                } else if (!bt) {
                    LOG3("  - no backtrack point found!");
                } else if (backtrack_count >= MaxBacktracksPerPipe) {
                    LOG3("  - too many backtracks!");
                } else {
                    LOG3("  - backtrack trying " << (*bt)->name << " in stage " << (*bt)->stage);
                    bt_mgmt.backtrack_to(bt);
                    continue; } }
        }

        if (best->table) {
            int dep_chain = self.deps.stage_info[best->table].dep_stages_control_anti;
            if ((best->stage + dep_chain >= Device::numStages()) &&
                (backtrack_count <= MaxBacktracksPerPipe)) {
                if (bt_mgmt.find_backtrack_solution(best, dep_chain)) {
                    continue;
                }
            }
        }
        self.add_starter_pistols(placed, &best, current);
        placed = place_table(work, best);

        if (!self.options.disable_table_placement_backfill) {
            for (auto p : trial) {
                /* Look for tables that are were placeable in this stage and are now not placeable
                 * and remember them for future backfilling.  Currently restricted to tables with
                 * no control dependent tables and no gateway merged */
                if (p != placed && p->stage == placed->stage && !work.count(p->group) &&
                    !p->need_more && !p->gw && p->table->next.empty()) {
                    LOG2("potential backfill " << p->name << " before " << placed->name);
                    backfill.add(p, placed); } } }

        if (placed->need_more)
            partly_placed.insert(placed->table);
        else
            partly_placed.erase(placed->table);
    }
    bt_mgmt.select_best_final_solution();
    LOG1("Table placement placed " << count(placed) << " tables in " <<
         (placed ? placed->stage+1 : 0) << " stages");
    return placed;
}

bool DecidePlacement::preorder(const IR::BFN::Pipe *pipe) {
    LOG_FEATURE("stage_advance", 2, "Stage advance for pipe " << pipe->canon_name() << " " <<
                self.summary.getActualStateStr() <<
                (self.ignoreContainerConflicts ? " " : " not ") << "ignoring container conflicts");

    const Placed *placed = nullptr;
    self.success = true;
    if (self.summary.is_table_replay()) {
        std::tie(self.success, placed) = alt_table_placement(pipe);
    } else {
        placed = default_table_placement(pipe);
    }

    if (self.success && placed) {
        LOG_FEATURE("stage_advance", 2,
                    "Stage " << placed->stage << IndentCtl::indent << Log::endl <<
                    StageSummary(placed->stage, placed) << IndentCtl::unindent); }

    self.summary.setAllStagesResources(get_total_stage_use(placed));

    self.placement = placed;
    self.table_placed.clear();
    for (auto p = self.placement; p; p = p->prev) {
        LOG2("  Table " << p->name << " logical id 0x" << hex(p->logical_id) <<
             " entries=" << p->entries << " stage=" << p->stage);
        for (auto &att : p->attached_entries)
            LOG3("    attached table " << att.first->name << " entries=" << att.second.entries <<
                 (att.second.need_more ? " (need_more)" : ""));
        BUG_CHECK(p->name == p->table->name, "table name mismatch %s != %s", p->name,
                  p->table->name);
        BUG_CHECK(p->need_more || self.table_placed.count(p->name) == 0,
              "Table %s placed more than once?", p->name);
        self.table_placed.emplace_hint(self.table_placed.find(p->name), p->name, p);
        if (p->gw) {
            LOG2("  Gateway " << p->gw->name << " is also logical id 0x" << hex(p->logical_id));
            BUG_CHECK(p->need_more || self.table_placed.count(p->gw->name) == 0,
                      "Gateway %s placed more than once?", p->gw->name);
            self.table_placed.emplace_hint(self.table_placed.find(p->gw->name), p->gw->name, p); } }
    LOG1("Finished table placement decisions " << pipe->canon_name());
    return false;
}

std::pair<bool, const DecidePlacement::Placed*>
DecidePlacement::alt_table_placement(const IR::BFN::Pipe *pipe) {
    LOG1("Table placement starting on " << pipe->canon_name() << " with ALT PLACEMENT approach");
    LOG3(TableTree("ingress"_cs, pipe->thread[INGRESS].mau) <<
         TableTree("egress"_cs, pipe->thread[EGRESS].mau) <<
         TableTree("ghost"_cs, pipe->ghost_thread.ghost_mau) );
    const Placed *placed = nullptr;

    LOG5("Table Summary Contents: " << self.summary.getActualStateStr());
    LOG5(self.summary);
    LOG5("Mau Backtracker Contents: ");
    self.mau_backtracker.printTableAlloc();
    LOG3("Alt Table Final Ordering: ");
    for (auto &pts : self.summary.getPlacedTables()) {
        LOG3("Global ID : " << pts.first << " " << *pts.second);
    }

    // Store all tables and their need_more (entries) status for quick access
    // Avoids looping over all tables in each iteration
    std::unordered_map<cstring, bool> need_more_tables;
    for (auto &pt : self.summary.getPlacedTables()) {
        auto alt_stage_to_place = pt.second->stage;

        TablePlacement::GatewayMergeChoices gmc;
        StageUseEstimate current = get_current_stage_use(placed);
        auto ptName = pt.second->internalTableName;
        const auto alt_table_to_place = self.getTblByName(ptName);

        const IR::MAU::Table* alt_cond_to_place = nullptr;
        const IR::MAU::Table* alt_try_place_table = nullptr;
        if (pt.second->gatewayName) {
            alt_cond_to_place = self.getTblByName(pt.second->gatewayName);
            gmc[alt_table_to_place] = pt.second->gatewayMergeCond;
        }
        if (alt_cond_to_place) {
            alt_try_place_table = alt_cond_to_place;
        } else {
            alt_try_place_table = alt_table_to_place;
        }

        if (!alt_try_place_table && !is_starter_pistol_table(ptName)) {
            warning("Cannot find table called %s(%s) during alt table placement round",
                    pt.second->tableName, pt.second->internalTableName);
            continue;
        }

        // Setup starter pistol table if required
        if (is_starter_pistol_table(ptName)) {
            LOG1("Adding starter pistol : " << ptName);
            placed = self.add_starter_pistols(placed, nullptr, current);
            continue;
        }

        // Check if table previously placed and done
        // E.g.
        // Table Franktown - 4096 entries
        // Placement Round 2 - Post Real PHV Allocation
        // --------------------------- ------------
        // |    Stage    |   Entries  |   Total    |
        // --------------------------- ------------
        // |     4       |    1024    |    1024    |
        // --------------------------- ------------
        // |     5       |    1024    |    2048    |
        // --------------------------- ------------
        // |     6       |    2048    |    4096    |
        // --------------------------- ------------
        //
        // Placement Replay Round 1 - Post Placement Round 2
        // --------------------------- ------------
        // |    Stage    |   Entries  |   Total    |
        // --------------------------- ------------
        // |     4       |    2048    |    2048    |
        // --------------------------- ------------
        // |     5       |    2048    |    4096    |
        // --------------------------- ------------
        // |     6       |     -      |     -      |
        // --------------------------- ------------
        // Table takes only 2 stages in replay round to place all entries.
        // Skip for stage 6.
        // NOTE: Skip unless there is an explicit stage pragma as it implies the table must have
        // specified entries on that stage.
        int stage_pragma = alt_try_place_table->get_provided_stage(alt_stage_to_place);
        // Table placed before
        if (need_more_tables.count(alt_try_place_table->name)
            // Table does not have need_more set
            && (!need_more_tables[alt_try_place_table->name])
            // Table does not have a stage pragma for current stage
            && (stage_pragma != alt_stage_to_place)) {
            LOG3("Skipping table " << alt_try_place_table->name
                                   << " as it has been entirely placed already");
            continue;
        }

        auto pl_vec = self.try_place_table(alt_try_place_table,
                                            placed, current, gmc, pt.second);
        LOG1("    Pl vector: " << pl_vec);
        if (pl_vec.size() == 0) {
            LOG3("Alt placement cannot place table " << alt_try_place_table->name);
            return std::make_pair(false, placed);
        }

        // Update placed object with new placed table
        placed = pl_vec[0];
        BUG_CHECK(placed->table,
                "Alt placement has no table present for placed object");

        // NOTE: A BUG_CHECK enabled here would enforce a hard table
        // ordering approach where the 2nd round of Table Placement __has__
        // to follow the first round during ALT_PHV_ALLOC.
        //
        // However, we make this a warning to allow flexibility in table
        // placement and let it explore alternate placements while trying to
        // hint at table ordering.
        //
        // We eventually might want to move this warning to a log message
        // when alt table placement is more robust. This warning is
        // currently for informational purposes only to identify places
        // where table placement deviates from expected based on previous
        // round and not that useful to end user.
        if (alt_stage_to_place != placed->stage) {
            warning(BFN::ErrorType::WARN_TABLE_PLACEMENT,
                "Alt Table Placement: Placed table %s(%s) in stage %d expected placement "
                "in stage from previous round - %d\n Placement will continue.",
                placed->name, placed->table->name, placed->stage, alt_stage_to_place);
        }
        if (LOGGING(1)) {
            auto tbl_log_str = placed->table
                                   ? placed->name + " ( " + placed->table->externalName() + " ) "
                                   : placed->name;
            auto gw_log_str = placed->gw ? " (with gw " + placed->gw->name + ", result tag " +
                                               placed->gw_result_tag + ")"
                                         : "";
            auto need_more_match_str = placed->need_more_match
                                           ? " (need more match)"
                                           : placed->need_more ? " (need more)" : "";
            LOG1("placing " << placed->entries << " entries of " << tbl_log_str << gw_log_str
                            << " in stage " << placed->stage << "(" << hex(placed->logical_id)
                            << ") " << placed->use.format_type << need_more_match_str);
        }

        need_more_tables[alt_table_to_place->name] = placed->need_more;
    }
    LOG1("Alt placement finished all table placement decisions on pipe " << pipe->canon_name());
    return std::make_pair(true, placed);
}

void TablePlacement::reject_placement(const Placed *of, choice_t reason, const Placed *better) {
    auto &rp = rejected_placements[of->name][better->name];
    rp.reason = reason;
    rp.stage = of->stage;
    rp.entries = of->entries;
    rp.attached_entries = of->attached_entries;
}

/* Human-readable strings for the choice_t enum used to return
 * is_better decision info.
 */
std::ostream &operator<<(std::ostream &out, TablePlacement::choice_t choice) {
    static const char* choice_names[] = {
        "earlier stage calculated",
        "earlier stage provided",
        "more stages needed",
        "completes more shared tables",
        "user-provided priority",
        "longer downward prop control-included dependence tail chain",
        "longer local control-included dependence tail chain",
        "longer control-excluded dependence tail chain",
        "fewer total dependencies",
        "longer downward dominance frontier dependence chain",
        "fewer total dependencies in dominance frontier",
        "direct control dependency difference",
        "control dom set is placeable in this stage",
        "control dom set has more placeable tables",
        "average chain length of control dom set",
        "default choice"
         };
    if (choice < sizeof(choice_names) / sizeof(choice_names[0])) {
        out << choice_names[choice];
    } else {
        out << "unknown choice <0x" << hex(choice) << ">";
    }
    return out;
}

IR::Node *TransformTables::preorder(IR::BFN::Pipe *pipe) {
    always_run_actions.clear();
    return pipe;
}

IR::Node *TransformTables::postorder(IR::BFN::Pipe *pipe) {
    self.tblInfo.clear();
    self.tblByName.clear();
    self.seqInfo.clear();
    self.table_placed.clear();
    LOG3("table placement completed " << pipe->canon_name());
    LOG3(TableTree("ingress"_cs, pipe->thread[INGRESS].mau) <<
         TableTree("egress"_cs, pipe->thread[EGRESS].mau) <<
         TableTree("ghost"_cs, pipe->ghost_thread.ghost_mau));
    BUG_CHECK(always_run_actions.empty(), "Inconsistent always_run list");
    return pipe;
}

void TransformTables::table_set_resources(IR::MAU::Table *tbl, const TableResourceAlloc *resources,
                                const int entries) {
    tbl->resources = resources;
    tbl->layout.entries = entries;
    if (!tbl->ways.empty()) {
        BUG_CHECK(errorCount() > 0 || resources->memuse.count(tbl->unique_id()),
                  "Missing resources for %s", tbl);
        if (!tbl->layout.atcam && resources->memuse.count(tbl->unique_id())) {
            auto &mem = resources->memuse.at(tbl->unique_id());
            // mismatch between tbl->ways and mem.ways is an error, but is diagnosed elsewhere
            // and we don't want to crash here.
            unsigned i = 0;
            for (auto &w : tbl->ways) {
                if (i < mem.ways.size())
                    w.entries = mem.ways[i++].size * 1024 * w.match_groups;
                else
                    w.entries = 0;
            }
        }
    }
}

/* Sets the layout and ways for a table from the selected table layout option
   from table placement */
static void select_layout_option(IR::MAU::Table *tbl, const LayoutOption *layout_option) {
    tbl->layout = layout_option->layout;
    if (!layout_option->layout.ternary) {
        tbl->ways.resize(layout_option->way_sizes.size());
        int index = 0;
        for (auto &way : tbl->ways) {
            way = layout_option->way;
            way.entries = way.match_groups * 1024 * layout_option->way_sizes[index];
            index++;
        }
    }
}

/* Adds the potential ternary tables necessary for layout options */
static void add_attached_tables(IR::MAU::Table *tbl, const LayoutOption *layout_option,
        const TableResourceAlloc *resources) {
    if (resources->has_tind()) {
        BUG_CHECK(layout_option->layout.no_match_miss_path() ||
                  layout_option->layout.ternary, "Illegally allocating a ternary indirect");
        LOG3("  Adding Ternary Indirect table to " << tbl->name);
        auto *tern_indir = new IR::MAU::TernaryIndirect(tbl->name);
        tbl->attached.push_back(new IR::MAU::BackendAttached(tern_indir->srcInfo, tern_indir));
    }
    if (layout_option->layout.direct_ad_required()) {
        LOG3("  Adding Action Data Table to " << tbl->name);

        // TODO: Add pipe prefix with multi pipe support
        cstring ad_name = tbl->match_table->externalName();
        if (tbl->layout.pre_classifier)
            ad_name = ad_name + "_preclassifier";
        ad_name = ad_name + "$action";
        auto *act_data = new IR::MAU::ActionData(IR::ID(ad_name));
        act_data->size = layout_option->entries;
        act_data->direct = true;
        auto *ba = new IR::MAU::BackendAttached(act_data->srcInfo, act_data);
        ba->addr_location = IR::MAU::AddrLocation::DIRECT;
        ba->pfe_location = IR::MAU::PfeLocation::DEFAULT;
        ba->entries = act_data->size;
        tbl->attached.push_back(ba);
    }
}

/** Similar to splitting a IR::MAU::Table across stages, this is splitting an ATCAM
 *  IR::MAU::Table object into the multiple logical tables specified by the table placement
 *  algorithm.  Like split tables, these tables are chained in a hit miss chain in order
 *  to maintain priority.
 *
 *  For an ATCAM table split across stages, a pointer to the last table in the chain is saved
 *  in the last pointer to chain the last table in the current stage to the first table in
 *  the next stage
 */
IR::MAU::Table *TransformTables::break_up_atcam(IR::MAU::Table *tbl,
        const TablePlacement::Placed *placed,
        int stage_table, IR::MAU::Table **last) {
    IR::MAU::Table *rv = nullptr;
    IR::MAU::Table *prev = nullptr;
    int logical_tables = placed->use.preferred()->logical_tables();

    BUG_CHECK(stage_table == placed->stage_split, "mismatched stage table id");
    LOG3("atcam table: " << tbl->name << " has " << logical_tables << "logical tables");
    int atcam_entries_in_stage = 0;
    for (int lt = 0; lt < logical_tables; lt++) {
        int entries = placed->use.preferred()->partition_sizes[lt] * Memories::SRAM_DEPTH;
        atcam_entries_in_stage += entries;
        LOG3("atcam table: " << tbl->name << " partition has " << entries << " entries");
    }
    LOG3("atcam table: " << tbl->name << " has " << atcam_entries_in_stage << " entries");

    for (int lt = 0; lt < logical_tables; lt++) {
        auto *table_part = tbl->clone();
        if (lt != 0)
            table_part->remove_gateway();
        // Clear gateway_name for the split table
        table_part->set_global_id(placed->logical_id + lt);
        table_part->logical_split = lt;
        table_part->logical_tables_in_stage = logical_tables;
        table_part->atcam_entries_in_stage = atcam_entries_in_stage;
        auto rsrcs = placed->resources.clone()->rename(tbl, stage_table, lt);
        int entries = placed->use.preferred()->partition_sizes[lt] * Memories::SRAM_DEPTH;
        table_set_resources(table_part, rsrcs, entries);
        if (!rv) {
            rv = table_part;
            BUG_CHECK(!prev, "First logical table for %s is not first?", tbl->name);
        } else {
            prev->next["$try_next_stage"_cs] = new IR::MAU::TableSeq(table_part);
            prev->next.erase("$miss"_cs);
        }

        if (last != nullptr)
            *last = table_part;
        prev = table_part;
    }
    return rv;
}

/** Splits a single dleft table into a Vector of tables.  Unlike ATCAM tables or multi-stage
 *  tables, predication is not used to enable or disable tables.   Instead all run, and the
 *  learn/match method is used to determine which table to run.  Thus, the algorithm differs
 *  from the hit miss chaining
 */
IR::Vector<IR::MAU::Table> *TransformTables::break_up_dleft(IR::MAU::Table *tbl,
        const TablePlacement::Placed *placed, int stage_table) {
    auto dleft_vector = new IR::Vector<IR::MAU::Table>();
    int logical_tables = placed->use.preferred()->logical_tables();
    BUG_CHECK(stage_table == placed->stage_split, "mismatched stage table id");

    for (int lt = 0; lt < logical_tables; lt++) {
        auto *table_part = tbl->clone();

        if (lt != 0)
            table_part->remove_gateway();
        table_part->set_global_id(placed->logical_id + lt);
        table_part->logical_split = lt;
        table_part->logical_tables_in_stage = logical_tables;

        const IR::MAU::StatefulAlu *salu = nullptr;
        for (auto back_at : tbl->attached) {
            salu = back_at->attached->to<IR::MAU::StatefulAlu>();
            if (salu != nullptr)
                break;
        }
        BUG_CHECK(salu, "No salu found associated with the attached table, %1%", tbl->name);

        auto rsrcs = placed->resources.clone()->rename(tbl, stage_table, lt);
        int per_row = RegisterPerWord(salu);
        int entries = placed->use.preferred()->dleft_hash_sizes[lt] * Memories::SRAM_DEPTH
                      * per_row;
        table_set_resources(table_part, rsrcs, entries);
        if (lt != logical_tables - 1)
            table_part->next.clear();

        dleft_vector->push_back(table_part);
    }
    return dleft_vector;
}

void TablePlacement::setup_detached_gateway(IR::MAU::Table *tbl, const Placed *placed) {
    tbl->remove_gateway();
    BUG_CHECK(placed->entries == 0, "match entries present");
    for (auto *ba : placed->table->attached) {
        if (ba->attached->direct || placed->attached_entries.at(ba->attached).entries == 0)
            continue;
        if (auto *ena = att_info.split_enable(ba->attached, tbl)) {
            tbl->gateway_rows.emplace_back(
                new IR::Equ(ena, new IR::Constant(1)), cstring());
            tbl->gateway_cond = ena->toString();
        }
    }
}


/**
 * When merging a match table and a gateway, the next map of these tables have to be merged.
 * Consider the following example, where the condition will be merged with table t2:
 *
 *    if (cond) {
 *        t1.apply();
 *        t2.apply();
 *        t3.apply();
 *    }
 *
 * The original IR graph would look something like:
 *
 * cond
 *   |
 *   | $true
 *   |
 * [ t1, t2, t3 ]
 *
 * Now after the merge, the IR graph would look like:
 *
 *  cond
 *   t2
 *   |
 *   | $default
 *   |
 * [ t1, t3 ]
 *
 * In this case, when the condition cond is true, the gateway will then run the table t2.
 *
 * Now another example:
 *
 *     if (cond) {
 *         t1.apply();
 *         t2.apply();
 *         t3.apply();
 *     } else {
 *         t4.apply();
 *     }
 *              cond
 *              / \
 *       $true /   \ $false
 *            /     \
 * [ t1, t2, t3 ]    [ t4 ]
 *
 * After the linkage:
 *
 *              cond
 *              t2
 *              / \
 *    $default /   \ $false
 *            /     \
 * [ t1, t3 ]       [ t4 ]
 *
 * A $default path is added for the replacement branch, as the $true branch is removed from
 * the gateway rows, instead representing to run table.
 *
 * Now this gets a bit trickier if the merged table has control flow off of it as well.  Say t2
 * is an action chain, e.g.:
 *
 *    if (cond) {
 *        t1.apply();
 *        switch (t2.apply().action_run()) {
 *            a1 : { t2_a1.apply(); }
 *        }
 *        t3.apply();
 *    }
 *
 * Original IR graph:
 *
 * cond
 *   |
 *   | $true
 *   |
 * [ t1, t2, t3 ]
 *       |
 *       | a1
 *       |
 *     [ t2_al ]
 *
 * Post merge:
 *              cond
 *              t2
 *              / \
 *    $default /   \ a1
 *            /     \
 * [ t1, t3 ]       [ t2_a1, t1, t3 ]
 *
 * A default pathway has been added, and to all pathways, the tables on the same sequence are
 * added.  If t2 had a default pathway already, then, simply t1 and t3 would have been added to
 * the next table graph.
 *
 * Last corner case is instead of an action chain, the table is a hit or miss:
 *
 *    if (cond) {
 *        t1.apply();
 *        if (t2.apply().miss) {
 *            t2_miss.apply();
 *        }
 *        t3.apply();
 *    }
 *
 * Original IR graph:
 *
 * cond
 *   |
 *   | $true
 *   |
 * [ t1, t2, t3 ]
 *       |
 *       | $miss
 *       |
 *     [ t2_miss ]
 *
 * Now, a $default pathway cannot be added to a hit miss table, currently in the IR.  Thus instead
 * the IR will be transformed to:
 *
 *              cond
 *              t2
 *              / \
 *        $hit /   \ $miss
 *            /     \
 * [ t1, t3 ]       [ t2_miss, t1, t3 ]
 *
 * This ensure that the tables t1 and t3 are run on both pathways.
 */
void TransformTables::merge_match_and_gateway(IR::MAU::Table *tbl,
        const TablePlacement::Placed *placed, IR::MAU::Table::Layout &gw_layout) {
    auto match = placed->table;
    BUG_CHECK(match && tbl->conditional_gateway_only() && !match->uses_gateway(),
        "Table IR is not set up in the preorder");
    LOG3("folding gateway " << tbl->name << " onto " << match->name);

    tbl->gateway_name = tbl->name;
    tbl->gateway_result_tag = placed->gw_result_tag;
    tbl->name = match->name;
    tbl->srcInfo = match->srcInfo;
    for (auto &gw : tbl->gateway_rows)
        if (gw.second == placed->gw_result_tag)
            gw.second = cstring();

    // Clone IR Information
    tbl->match_table = match->match_table;
    tbl->match_key = match->match_key;
    tbl->actions = match->actions;
    tbl->attached = match->attached;
    tbl->entries_list = match->entries_list;
    tbl->created_during_tp = match->created_during_tp;
    tbl->is_compiler_generated = match->is_compiler_generated;
    tbl->has_dark_init = match->has_dark_init;
    tbl->random_seed = match->random_seed;
    tbl->dynamic_key_masks = match->dynamic_key_masks;

    // Generate the correct table layout from the options
    gw_layout = tbl->layout;

    BUG_CHECK(gw_layout.hash_action == false,
              "unexpected value for layout.hash_action on gateway");
    gw_layout.hash_action = false;

    // Remove the conditional sequence under the branch
    auto *seq = tbl->next.at(placed->gw_result_tag)->clone();
    tbl->next.erase(placed->gw_result_tag);
    bool cond_seq_found = false;
    if (seq->tables.size() != 1) {
        for (auto it = seq->tables.begin(); it != seq->tables.end(); it++) {
            if (*it == match) {
                seq->tables.erase(it);
                cond_seq_found = true;
                break;
            }
        }
    } else {
        if (seq->tables[0] == match)
            cond_seq_found = true;
        seq = 0;
    }
    BUG_CHECK(cond_seq_found,
        "failed to find match table %s in next sequence of table %s",
        match->name, tbl->name);

    // Add all of the tables in the same sequence (i.e. t1 and t3) to all of the branches
    for (auto &next : match->next) {
        BUG_CHECK(tbl->next.count(next.first) == 0, "Added to another table");
        if (seq) {
            auto *new_next = next.second->clone();
            for (auto t : seq->tables)
                new_next->tables.push_back(t);
            tbl->next[next.first] = new_next;
        } else {
            tbl->next[next.first] = next.second;
        }
    }

    // Create the missing $hit, $miss, or $default branch if the program does not have it
    if (match->hit_miss_p4()) {
        if (match->next.count("$hit"_cs) == 0 && seq)
            tbl->next["$hit"_cs] = seq;
        if (match->next.count("$miss"_cs) == 0 && seq)
            tbl->next["$miss"_cs] = seq;
    } else if (!match->has_default_path() && seq) {
        tbl->next["$default"_cs] = seq;
    }

    for (auto &gw : tbl->gateway_rows)
        if (gw.second && !tbl->next.count(gw.second))
            tbl->next[gw.second] = new IR::MAU::TableSeq();
}

void TablePlacement::find_dependency_stages(const IR::MAU::Table *tbl,
            std::map<int, ordered_map<const Placed *,
                                      DependencyGraph::dependencies_t>> &earliest_stage) const {
    auto &need_different_stage = deps.happens_phys_after_map.at(tbl);
    for (auto *pred : deps.happens_logi_after_map.at(tbl)) {
        auto dep_kind = deps.get_dependency(tbl, pred);
        if (!dep_kind) continue;  // not a real dependency?
        auto pl = find_placed(pred->name);
        // BUG_CHECK(pl != table_placed.end(), "no placement for table %s", pred->name);
        if (pl == table_placed.end()) continue;
        BUG_CHECK(pl->second->table == pred || pl->second->gw == pred, "found wrong placement(%s) "
                  "for %s", pl->second->name, pred->name);
        if (pl->second->table == pred) {
            while (pl->second->need_more_match) {
                ++pl;
                // Do not run the BUG_CHECK when placement has failed. For failed placement table
                // can be incompletely placed. This will cause compilation to stop. However for non
                // failed placement this is an invalid condition and should be checked.
                // NOTE: Currently, this function (and the BUG_CHECKs) are only run when logging is
                // enabled.
                if (((pl == table_placed.end() || pl->second->table != pred) && (!success))
                    // TBD Need a better way to handle atcams which trigger BUG_CHECK on
                    // some alt-phv-alloc cases
                    || (success && options.alt_phv_alloc &&
                        pl->second->table->layout.pre_classifier)
                    || (options.alt_phv_alloc && !pl->second->table)) {
                    --pl;
                    break;
                }
                if (summary.getActualState() == State::SUCCESS) {
                    BUG_CHECK(pl != table_placed.end() && pl->second->table == pred,
                              "incomplete placement for table %s", pred->name);
                }
            }
        }
        int stage = pl->second->stage;
        if (need_different_stage.count(pred)) ++stage;
        earliest_stage[stage].emplace(pl->second, dep_kind);
    }
}

namespace {
class SplitEntriesList {
    const IR::EntriesList* list;
    IR::Vector<IR::Entry>::const_iterator next;
    size_t available = 0;

 public:
    explicit SplitEntriesList(const IR::EntriesList* list) : list(list) {
        if (list) {
            next = list->entries.begin();
            available = list->size();
        }
    }
    const IR::EntriesList* split_off(size_t num_entries) {
        if (!available)
            return nullptr;
        if (num_entries >= available) {
            if (next == list->entries.begin()) {
                // All entries fit in this table.
                available = 0;
                return list;
            }
            num_entries = available;
        }

        auto first = next;
        next = first + num_entries;
        available -= num_entries;
        IR::Vector<IR::Entry> static_entries;
        static_entries.insert(static_entries.end(), first, next);
        return new IR::EntriesList(static_entries);
     }
};
}  // namespace

IR::Node *TransformTables::preorder(IR::MAU::Table *tbl) {
    Log::TempIndent indent;
    LOG5("TransformTables preorder on " << tbl->name << indent);
    auto it = self.table_placed.find(tbl->name);
    if (it == self.table_placed.end()) {
        BUG_CHECK(errorCount() > 0, "Trying to place a table %s that was never placed", tbl->name);
        return tbl; }
    if (tbl->is_placed())
        return tbl;

    const TablePlacement::Placed* pl = it->second;

    if (LOGGING_FEATURE("stage_advance", 2)) {
        // Skip starter pistol tables
        if (!is_starter_pistol_table(pl->table->name)) {
            // look for tables that were delayed (not placed in the earliest stage they could have
            // been based on depedencies) and log that
            std::map<int, ordered_map<const TablePlacement::Placed *,
                                      DependencyGraph::dependencies_t>> earliest_stage;
            self.find_dependency_stages(pl->table, earliest_stage);
            if (pl->gw) self.find_dependency_stages(pl->gw, earliest_stage);
            if (earliest_stage.empty()) {
                if (pl->stage != 0)
                    LOG_FEATURE("stage_advance", 2, "Table " << pl->name << " with no "
                                "predecessors delayed until stage " << pl->stage);
            } else if (earliest_stage.rbegin()->first < pl->stage) {
                LOG_FEATURE("stage_advance", 2, "Table " << pl->name << " delayed to stage " <<
                            pl->stage << " from stage " << earliest_stage.rbegin()->first);
                ordered_set<const TablePlacement::Placed *> filter;
                // filter out duplicates due to multiple kinds of dependencies
                for (auto i = earliest_stage.end(); i != earliest_stage.begin();) {
                    --i;
                    for (auto *p : filter) i->second.erase(p);
                    if (i->second.empty()) {
                        i = earliest_stage.erase(i);
                    } else {
                        for (auto &p : i->second) filter.insert(p.first); } }
                for (auto i = earliest_stage.rbegin(); i != earliest_stage.rend(); ++i) {
                    for (auto &p : i->second) {
                        LOG_FEATURE("stage_advance", 2, "  stage " << i->first << ": " <<
                                    "dependency (" << p.second << ") on " << p.first->name <<
                                    " in stage " << p.first->stage);
                    }
                }
            }
            for (auto &rr : self.rejected_placements[tbl->name]) {
                if (rr.second.stage < pl->stage || rr.second.entries > pl->entries ||
                    rr.second.attached_entries > pl->attached_entries) {
                    LOG_FEATURE("stage_advance", 3, "  - preferred " << rr.first << "(" <<
                                rr.second.reason << ") over " << rr.second.entries << " of " <<
                                tbl->name << " in stage " << rr.second.stage);
                }
            }
        }
    }

    // FIXME: Currently the gateway is laid out for every table, so I'm keeping the information
    // in split tables.  In the future, there should be no gw_layout for split tables
    // FIXME -- gw_layout should be unnecessary and should go away completely
    IR::MAU::Table::Layout gw_layout;
    bool gw_only = true;
    bool gw_layout_used = false;
    ordered_map<cstring, const IR::MAU::TableSeq *> match_table_next;
    IR::MAU::TableSeq *gw_result_tag_seq = nullptr;

    if (pl->use.preferred() && pl->use.preferred()->layout.gateway_match) {
        tbl = self.lc.fpc.convert_to_gateway(tbl);
        gw_only = false;
    } else if (pl->gw && pl->gw->name == tbl->name) {
        // Fold gateway and match table together, deepcopy match table's next sequences.
        // If the merged table is split, all table parts beyond the first will use this copy.
        if (!pl->table->next.empty() && Device::currentDevice() != Device::TOFINO) {
            for (auto const &element : pl->table->next) {
                // Create new TableSeqs rather than clones to have new unique IDs.
                auto deepcopy = new IR::MAU::TableSeq();
                deepcopy->tables = element.second->tables;
                match_table_next[element.first] = deepcopy;
            }
        }
        // Store a clone of gw's gw_result_tag branch to be used if the merged table is split.
        gw_result_tag_seq =
            (tbl->next.count(pl->gw_result_tag)) ? tbl->next[pl->gw_result_tag]->clone() : nullptr;
        merge_match_and_gateway(tbl, pl, gw_layout);
        gw_only = false;
        gw_layout_used = true;
    } else if (pl->table->match_table) {
        gw_only = false;
    }

    if (tbl->is_always_run_action()) {
        tbl->stage_ = pl->stage;
        LOG4("\t Set stage for ARA " << tbl->name << " to " << tbl->stage());
    } else {
        tbl->set_global_id(pl->logical_id);
        LOG4("\t Set stage for table " << tbl->name << " to " << tbl->stage());
    }

    if (self.table_placed.count(tbl->name) == 1) {
        if (!gw_only) {
            select_layout_option(tbl, pl->use.preferred());
            add_attached_tables(tbl, pl->use.preferred(), &pl->resources);
            if (gw_layout_used)
                tbl->layout += gw_layout;
        }
        if (tbl->layout.atcam)
            return break_up_atcam(tbl, pl);
        else if (tbl->for_dleft())
            return break_up_dleft(tbl, pl);
        else
            table_set_resources(tbl, pl->resources.clone(), pl->entries);
        return tbl;
    }
    int stage_table = 0;
    int prev_entries = 0;
    IR::Vector<IR::MAU::Table> *rv = new IR::Vector<IR::MAU::Table>;
    IR::MAU::Table *prev = 0;
    IR::MAU::Table *atcam_last = nullptr;
    /* split the table into multiple parts per the placement */
    LOG1("splitting " << tbl->name << " across " << self.table_placed.count(tbl->name)
         << " stages");
    int deferred_attached = 0;
    for (auto *att : tbl->attached) {
        if (att->attached->direct) continue;
        if (!pl->attached_entries.at(att->attached).need_more) continue;
        // splitting a table with an un-duplicatable indirect attached table
        // allocate TempVar to propagate index from match to attachment stage
        ++deferred_attached;
        if (att->type_location == IR::MAU::TypeLocation::OVERHEAD)
            P4C_UNIMPLEMENTED("%s with multiple RegisterActions placed in separation stage from "
                              "%s; try using @stage to force them into the same stage",
                              tbl->match_table->name, att->attached->name); }
    if (deferred_attached > 1)
        P4C_UNIMPLEMENTED("Splitting %s with multiple indirect attachements not supported",
                          tbl->match_table->name);

    int priority = 0;
    auto tbl_entries_list = SplitEntriesList{tbl->entries_list};
    for (const TablePlacement::Placed *pl : ValuesForKey(self.table_placed, tbl->name)) {
        auto *table_part = tbl->clone();

        // Divide the entries amongst the table_parts.
        table_part->entries_list = tbl_entries_list.split_off(pl->entries);

        // Set the value of the first entry's priority field in case
        // this table_part has some static entries.
        table_part->first_entry_list_priority = priority;
        priority += pl->entries;

        // When a gateway is merged against a split table, only the first table part created
        // from the split has the merged gateway.
        if (!rv->empty())
            table_part->remove_gateway();

        if (stage_table != 0) {
            BUG_CHECK(!rv->empty(), "failed to attach first stage table");
            BUG_CHECK(stage_table == pl->stage_split, "Splitting table %s on stage %d cannot be "
                      "resolved for stage table %d", table_part->name, pl->stage, stage_table); }
        select_layout_option(table_part, pl->use.preferred());
        add_attached_tables(table_part, pl->use.preferred(), &pl->resources);
        if (gw_layout_used)
            table_part->layout += gw_layout;
        table_part->set_global_id(pl->logical_id);
        table_part->stage_split = pl->stage_split;
        TableResourceAlloc *rsrcs = nullptr;

        if (pl->entries) {
            if (deferred_attached) {
                if (!pl->use.format_type.anyAttachedLaterStage())
                    error("Couldn't find a usable split format for %1% and couldn't place it "
                          "without splitting", tbl);
                for (auto act = table_part->actions.begin(); act != table_part->actions.end();) {
                    if ((act->second = self.att_info.get_split_action(act->second, table_part,
                                                                      pl->use.format_type)))
                        ++act;
                    else
                        act = table_part->actions.erase(act); }
                erase_if(table_part->attached, [pl](const IR::MAU::BackendAttached *ba) {
                    return pl->attached_entries.count(ba->attached) &&
                           pl->attached_entries.at(ba->attached).entries == 0; }); }
            if (table_part->layout.atcam) {
                table_part = break_up_atcam(table_part, pl, pl->stage_split, &atcam_last);
            } else {
                rsrcs = pl->resources.clone()->rename(tbl, pl->stage_split);
                table_set_resources(table_part, rsrcs, pl->entries);
            }
        } else {
            if (pl->use.format_type.matchThisStage())
                error("Couldn't find a usable split format for %1% and couldn't place it "
                      "without splitting", tbl);
            else
                BUG_CHECK(deferred_attached, "Split match from attached with no attached?");
            for (auto act = table_part->actions.begin(); act != table_part->actions.end();) {
                if ((act->second = self.att_info.get_split_action(act->second, table_part,
                                                                  pl->use.format_type)))
                    ++act;
                else
                    act = table_part->actions.erase(act); }
            rsrcs = pl->resources.clone()->rename(tbl, pl->stage_split);
            table_set_resources(table_part, rsrcs, pl->entries);
            table_part->match_key.clear();
            table_part->next.clear();
            table_part->suppress_context_json = true;
            self.setup_detached_gateway(table_part, pl);
            if (table_part->actions.size() != 1)
                P4C_UNIMPLEMENTED("split attached table with multiple actions");
            erase_if(table_part->attached, [pl](const IR::MAU::BackendAttached *ba) {
                return !pl->attached_entries.count(ba->attached) ||
                       pl->attached_entries.at(ba->attached).entries == 0; });
            for (size_t i = 0; i < table_part->attached.size(); i++) {
                auto ba_clone = new IR::MAU::BackendAttached(*(table_part->attached.at(i)));
                // This needs to be reset, as the backend attached is initialized as DEFAULT
                if (ba_clone->attached->is<IR::MAU::Counter>() ||
                    ba_clone->attached->is<IR::MAU::Meter>() ||
                    ba_clone->attached->is<IR::MAU::StatefulAlu>()) {
                    ba_clone->pfe_location = IR::MAU::PfeLocation::DEFAULT;
                    if (!ba_clone->attached->is<IR::MAU::Counter>())
                        ba_clone->type_location = IR::MAU::TypeLocation::DEFAULT;
                }
                table_part->attached.at(i) = ba_clone;
            }

            BUG_CHECK(!rv->empty(), "first stage has no match entries?");
            table_part->is_detached_attached_tbl = true;
            rv->push_back(table_part);
          }

        if (rv->empty()) {
            rv->push_back(table_part);
            BUG_CHECK(!prev, "didn't add prev stage table?");
        } else if (pl->entries) {
            // Connect all split table parts together so missing on a previous part leads
            // to trying the next part.
            //
            // If split table is merged with a gateway, the "$true" sequence of gateway should
            // still always happen if the gateway condition is met, regardless of hitting
            // or missing the first part of the split table it was merged with. Append "$true"
            // sequence to "$try_next_stage" sequence, which happens on miss, of the first
            // table part. This might result in more than one path being executed in future
            // stages, so this fix is exclusive from Tofino 2 onwards.
            //
            // FIXME: Long term solution could be the following for these types of actions:
            //    - Clone all actions and set them all to hit_only
            //    - Create a noop action as miss_only
            //    - Get rid of the $try_next_stage and just go through the standard hit/miss
            // Separate control flow processing for try_next_stage vs miss
            auto try_next_stage_seq = new IR::MAU::TableSeq(table_part);
            if (gw_result_tag_seq && Device::currentDevice() != Device::TOFINO) {
                for (auto table : gw_result_tag_seq->tables) {
                    // Remove table_part, as try_next_stage_seq already includes it.
                    if (table->name == table_part->name) {
                        gw_result_tag_seq->tables.erase(remove(gw_result_tag_seq->tables.begin(),
                                                         gw_result_tag_seq->tables.end(),
                                                         table),
                                                  gw_result_tag_seq->tables.end());
                        break;
                    }
                }
                try_next_stage_seq->tables.insert(
                    try_next_stage_seq->tables.end(),
                    gw_result_tag_seq->tables.begin(),
                    gw_result_tag_seq->tables.end());
                gw_result_tag_seq = nullptr;
            }
            // Since the gateway is handled by the first table part, all subsequent parts
            // should use a deepcopy of the original match table's next sequences.
            if (gw_layout_used && !table_part->layout.atcam &&
                Device::currentDevice() != Device::TOFINO) {
                table_part->next = match_table_next; }
            prev->next["$try_next_stage"_cs] = try_next_stage_seq;
            prev->next.erase("$miss"_cs);
        }

        // check for any attached tables not completely placed as of this stage and do any
        // address updates needed for later stages.
        for (auto &ba : table_part->attached) {
            auto *att = ba->attached;
            if (att->direct) continue;
            if (pl->attached_entries.at(att).entries == 0) continue;
            if (pl->attached_entries.at(att).need_more) {
                if (Device::currentDevice() == Device::TOFINO) {
                    error("splitting %s across stages not supported on Tofino", att);
                } else {
                    if (!ba->chain_vpn) {
                        // Need to chain the vpn to the next stage, which means we need to do
                        // the subword shift with meter_adr_shift and NOT with the hash_dist
                        for (auto &hdu : rsrcs->hash_dists) {
                            for (auto &alloc : hdu.ir_allocations) {
                                if (alloc.dest == IXBar::HD_METER_ADR) {
                                    BUG_CHECK(hdu.shift == 0, "Can't shift chained address in "
                                              "hash dist");
                                    break; } } }
                        clone_update(ba)->chain_vpn = true; } } } }

        stage_table++;
        prev = table_part;
        if (atcam_last)
            prev = atcam_last;
        prev_entries += pl->entries;
    }
    BUG_CHECK(!rv->empty(), "Failed to place any stage tables for %s", tbl->name);
    return rv;
}

IR::Node *TransformTables::postorder(IR::MAU::Table *tbl) {
    return Device::mauSpec().postTransformTables(tbl);
}

IR::Node *TransformTables::preorder(IR::MAU::BackendAttached *ba) {
    LOG5("TransformTables::preorder BackendAttached " << ba->attached->name);
    visitAgain();  // clone these for any table that has been cloned
    auto tbl = findContext<IR::MAU::Table>();
    if (!tbl || !tbl->resources) {
        BUG_CHECK(errorCount() > 0, "No table resources for %s", ba);
        return ba; }
    auto format = tbl->resources->table_format;

    if (ba->attached->is<IR::MAU::Counter>()) {
        if (format.stats_pfe_loc == IR::MAU::PfeLocation::OVERHEAD ||
            format.stats_pfe_loc == IR::MAU::PfeLocation::GATEWAY_PAYLOAD)
            ba->pfe_location = format.stats_pfe_loc;
    } else if (ba->attached->is<IR::MAU::Meter>() || ba->attached->is<IR::MAU::StatefulAlu>() ||
               ba->attached->is<IR::MAU::Selector>()) {
        if (format.meter_pfe_loc == IR::MAU::PfeLocation::OVERHEAD ||
            format.meter_pfe_loc == IR::MAU::PfeLocation::GATEWAY_PAYLOAD)
            ba->pfe_location = format.meter_pfe_loc;
        if (format.meter_type_loc == IR::MAU::TypeLocation::OVERHEAD ||
            format.meter_type_loc == IR::MAU::TypeLocation::GATEWAY_PAYLOAD)
            ba->type_location = format.meter_type_loc;
    }

    for (auto act : Values(tbl->actions)) {
        if (auto *sc = act->stateful_call(ba->attached->name)) {
            if (!sc->index) continue;
            if (auto *hd = sc->index->to<IR::MAU::HashDist>()) {
                ba->hash_dist = hd;
                ba->addr_location = IR::MAU::AddrLocation::HASH;
                break; } } }

    // If the table has been converted to hash action, then the hash distribution unit has
    // to be tied to the BackendAttached object for the assembly output
    if (tbl->layout.hash_action && ba->attached->direct) {
        for (auto &hd_use : tbl->resources->hash_dists) {
            for (auto &ir_alloc : hd_use.ir_allocations) {
                if (ba->attached->is<IR::MAU::Counter>() &&
                    ir_alloc.dest != IXBar::HD_STATS_ADR)
                    continue;
                if (ba->attached->is<IR::MAU::Meter>() &&
                    ir_alloc.dest != IXBar::HD_METER_ADR)
                    continue;
                if (ba->attached->is<IR::MAU::ActionData>() &&
                    ir_alloc.dest != IXBar::HD_ACTIONDATA_ADR)
                    continue;
                BUG_CHECK(ir_alloc.created_hd != nullptr, "Hash Action did not create a "
                    "HashDist object during allocation");
                ba->hash_dist = ir_alloc.created_hd;
                ba->addr_location = IR::MAU::AddrLocation::HASH;
            }
        }
    }

    // Update entries value on BackendAttached table
    auto it = self.table_placed.find(tbl->name);
    if (it != self.table_placed.end()) {
        const auto* pl = it->second;

        LOG6("Placed attached_entries " << pl->attached_entries);
        if (pl->attached_entries.size() > 0) {
            if (pl->attached_entries.count(ba->attached) > 0) {
                ba->entries = pl->attached_entries.at(ba->attached).entries;
            }
        }
    }

    return ba;
}

IR::Node *TransformTables::preorder(IR::MAU::TableSeq *seq) {
    if (getParent<IR::BFN::Pipe>())
        BUG_CHECK(always_run_actions.empty(), "inconsistent always_run list");

    // Inserting the starter pistol tables
    if (findContext<IR::MAU::Table>() == nullptr && seq->tables.size() > 0) {
        if (Device::currentDevice() == Device::TOFINO) {
            auto gress = seq->front()->gress;
            if (self.starter_pistol[gress] != nullptr) {
                seq->tables.push_back(self.starter_pistol[gress]);
            }
        }
    }
    return seq;
}

IR::Node *TransformTables::postorder(IR::MAU::TableSeq *seq) {
    /* move all always run actions to the top-level seq in the pipe threads */
    if (getParent<IR::BFN::Pipe>()) {
        seq->tables.append(always_run_actions);
        always_run_actions.clear();
    } else {
        for (auto it = seq->tables.begin(); it != seq->tables.end();) {
            if ((*it)->is_always_run_action()) {
                always_run_actions.insert(*it);
                it = seq->tables.erase(it);
            } else {
                ++it; } } }

    if (seq->tables.size() > 1) {
        std::stable_sort(seq->tables.begin(), seq->tables.end(),
            // Always Run Action will appear logically after all tables in its stage but before
            // all tables in subsequent stages
            [](const IR::MAU::Table *a, const IR::MAU::Table *b) -> bool {
                bool a_always_run = a->is_always_run_action();
                bool b_always_run = b->is_always_run_action();
                if (!a_always_run && !b_always_run) {
                    auto a_id = a->global_id();
                    auto b_id = b->global_id();
                    return a_id ? b_id ? *a_id < *b_id : true : false; }
                if (a->stage() != b->stage())
                    return a->stage() < b->stage();
                if (!a_always_run || !b_always_run)
                    return !a_always_run;
                return a->name < b->name;
        });
    }
    seq->deps.clear();  // FIXME -- not needed/valid?  perhaps set to all 1s?
    return seq;
}

/**
 * The goal of this pass is to merge all of the pre table-placement always run objects into a
 * single IR::MAU::Table.  Each always run action during table placement is placed
 * separately, however in the hardware, the action is a single IR::MAU::Action, and thus
 * must be merged together.
 *
 * The idea behind this is that if each always run action is indeed always run, then this table
 * must appear on every single IR pathway while honoring the IR rules of topological order.
 * Thus, by replacing a single always run table per stage and per gress will indeed place the
 * merged always run action in a legal place.  This is essentially what this pass does.
 *
 * The Scan pass is to find all always run action, and create a set of post table placement
 * tables that are merged per stage and gress.  The second step will find a single original
 * always run action per stage and gress, and replace it with this merged table.  All other
 * always run actions will be replaced with a nullptr.
 */
bool MergeAlwaysRunActions::Scan::preorder(const IR::MAU::Table *tbl) {
    if (tbl->is_always_run_action()) {
        AlwaysRunKey ark(tbl->stage(), tbl->gress);
        self.ar_tables_per_stage[ark].insert(tbl);
        LOG7("\t Merge Scan - ARA Table: " << tbl->name <<  "    Gress " << tbl->gress
             << "  phys stage:" << tbl->stage());
    }
    return true;
}

bool MergeAlwaysRunActions::Scan::preorder(const IR::MAU::Primitive *prim) {
    auto *tbl = findContext<IR::MAU::Table>();
    if (tbl != nullptr && tbl->is_always_run_action()) {
        for (uint32_t idx = 0; idx < prim->operands.size(); ++idx) {
            auto expr = prim->operands.at(idx);
            le_bitrange bits;
            PHV::Field* exp_f = self.self.phv.field(expr, &bits);
            if (!exp_f) continue;

            PHV::FieldSlice fslice = PHV::FieldSlice(exp_f, bits);

            if (idx != 0)
                self.read_fldSlice[tbl].insert(fslice);
            else
                self.written_fldSlice[tbl].insert(fslice);
        }

        LOG5("\tPrimitive " << *prim << "\n\t\t written slices: " << self.written_fldSlice <<
         "\n\t\t read slices: "  << self.read_fldSlice);
    }

    return true;
}

void MergeAlwaysRunActions::Scan::end_apply() {
    for (auto entry : self.ar_tables_per_stage) {
        const AlwaysRunKey& araKey = entry.first;
        auto& tables = entry.second;

        IR::MAU::Table *merged_table = new IR::MAU::Table(cstring(), araKey.gress);
        merged_table->stage_ = araKey.stage;
        merged_table->always_run = IR::MAU::AlwaysRun::ACTION;
        IR::MAU::Action *act = new IR::MAU::Action("$always_run_act"_cs);
        TableResourceAlloc *resources = new TableResourceAlloc();
        int ar_row = Device::alwaysRunIMemAddr() / 2;
        int ar_color = Device::alwaysRunIMemAddr() % 2;
        InstructionMemory::Use::VLIW_Instruction single_instr(bitvec(), ar_row, ar_color);
        resources->instr_mem.all_instrs.emplace("$always_run"_cs, single_instr);
        std::set<int> minDgStages;
        // Should really only be an Action and an instr_mem allocation
        for (auto tbl : tables) {
            LOG7("Physical stage for merge-scan table " << tbl->name << " : " << tbl->stage());
            BUG_CHECK(tbl->actions.size() == 1, "Always run tables can only have one action");
            const IR::MAU::Action *local_act = *(Values(tbl->actions).begin());
            for (auto instr : local_act->action)
                act->action.push_back(instr);
            resources->merge_instr(tbl->resources);
            for (auto stg : PhvInfo::minStages(tbl)) minDgStages.insert(stg);
        }

        auto tbl_to_replace = self.ar_replacement(araKey.stage, araKey.gress);
        merged_table->name = tbl_to_replace->name;
        merged_table->actions.emplace("$always_run_act"_cs, act);
        merged_table->resources = resources;
        self.merge_per_stage.emplace(araKey, merged_table);
        self.mergedARAwitNewStage = self.mergedARAwitNewStage || (minDgStages.size() > 1);
        self.merged_ar_minStages[araKey] = minDgStages;
        LOG7("\t Status of mergedARAwitNewStage:" << self.mergedARAwitNewStage);
    }
}

const IR::MAU::Table *MergeAlwaysRunActions::Update::preorder(IR::MAU::Table *tbl) {
    auto orig_tbl = getOriginal()->to<IR::MAU::Table>();
    if (!tbl->is_always_run_action())
        return tbl;
    AlwaysRunKey ark(tbl->stage(), tbl->gress);

    auto tbl_to_replace = self.ar_replacement(ark.stage, ark.gress);
    if (tbl_to_replace == orig_tbl) {
        return self.merge_per_stage.at(ark);
    } else {
        return nullptr;
    }
}

void MergeAlwaysRunActions::Update::end_apply() {
    // Print Fields involved in Always Run Actions
    for (auto pr : self.read_fldSlice) {
        LOG7("\t  Read slices for " << *(pr.first));
        for (auto sl : pr.second) {
            LOG7("\t\t" << *(sl.field()) << " num allocs: " << sl.field()->alloc_size());
        }
    }

    for (auto pr : self.written_fldSlice) {
        LOG7("\t  Written slices for " << *(pr.first));
        for (auto sl : pr.second) {
            LOG7("\t\t" << *(sl.field()) << " num allocs: " << sl.field()->alloc_size());
        }
    }

    // Update liveranges of AllocSlice's impacted by merged tables
    for (auto entry : self.ar_tables_per_stage) {
        const AlwaysRunKey& araKey = entry.first;
        auto& tables = entry.second;
        BUG_CHECK(self.merged_ar_minStages.count(araKey),
                  "No minStage info for ARA tables in stage %1% of gress %2%",
                  araKey.stage,
                  araKey.gress);

        LOG7("\t Merge Update - Phys stage (gress): " << araKey.stage
             << "(" << entry.first.gress << ") "
             << " #ARA tables: " << tables.size()
             << " #minStages:" << self.merged_ar_minStages[araKey].size());

        // Nothing to do if stage does not contain merged Always Run Action tables
        if (tables.size() <= 1)
            continue;

        // Nothing to do if merged tables have same minStage
        if (self.merged_ar_minStages.at(araKey).size() <= 1)
            continue;

        // Get table that will be replaced by merged table
        auto *merged_tbl = self.merge_per_stage.at(araKey);

        // Get max minStage of merged tables - This will be used as
        // the new minStage of the merged table
        int newStg = *(self.merged_ar_minStages.at(araKey).rbegin());

        std::stringstream ss;
        ss << "Merged tables: ";

        for (auto tbl : tables) {
            int oldStg = *(PhvInfo::minStages(tbl).begin());

            ss << tbl->externalName() << "  (stage:" << oldStg << ")  ";

            // If the minStage of the table has not changed there is nothing to do
            if (oldStg == newStg)
                continue;

            // Since (newStg != oldStg), we need to update all the affected AllocSlices
            // First look into the read slices
            for (auto fslice : self.read_fldSlice.at(tbl)) {
                PHV::Field *fld = const_cast<PHV::Field*>(fslice.field());
                le_bitrange rng = fslice.range();

                for (auto &alc_slice : fld->get_alloc()) {
                    if (!(alc_slice.field_slice().overlaps(rng)))
                        continue;

                    // The AlwaysRunAction table may be the last live stage of the slice or ...
                    if ((alc_slice.getLatestLiveness().first == oldStg) &&
                        (alc_slice.getLatestLiveness().second.isRead())) {
                        alc_slice.setLatestLiveness(
                            std::make_pair(newStg, alc_slice.getLatestLiveness().second));

                        LOG7("\tUpdate last stage from " << oldStg << " to " << newStg <<
                             " for read slice: " << alc_slice);
                        // Keep track of original slice LR END before merge
                        self.premergeLRend[&alc_slice][merged_tbl] = oldStg;
                    }
                }
            }

            // Then look into the written slices
            for (auto fslice : self.written_fldSlice.at(tbl)) {
                PHV::Field *fld = const_cast<PHV::Field*>(fslice.field());
                le_bitrange rng = fslice.range();

                for (auto &alc_slice : fld->get_alloc()) {
                    if (!(alc_slice.field_slice().overlaps(rng)))
                        continue;

                    // The AlwaysRunAction table may be the first live stage of the slice or ...
                    if ((alc_slice.getEarliestLiveness().first == oldStg) &&
                        (alc_slice.getEarliestLiveness().second.isWrite())) {
                        alc_slice.setEarliestLiveness(
                            std::make_pair(newStg, alc_slice.getEarliestLiveness().second));

                        LOG7("\tUpdate first stage from " << oldStg << " to " << newStg <<
                             " for written slice: " << alc_slice);
                        // Keep track of original slice LR START before merge
                        self.premergeLRstart[&alc_slice][merged_tbl] = oldStg;
                    }
                }
            }
        }

        ss << std::endl;
        LOG7(ss.str());
        PhvInfo::addMinStageEntry(merged_tbl, newStg, true);
    }

    LOG7("\t premergeLRend: ");
    for (auto slTblStg : self.premergeLRend) {
        LOG7("\t\t AllocSlice: " << slTblStg.first);
        for (auto tblStg : slTblStg.second) {
            LOG7("\t\t table " << tblStg.first->name << "  old LREnd stage: " << tblStg.second);
        }
    }

    LOG7("\t premergeLRstart: ");
    for (auto slTblStg : self.premergeLRstart) {
        LOG7("\t\t AllocSlice: " << slTblStg.first);
        for (auto tblStg : slTblStg.second) {
            LOG7("\t\t table " << tblStg.first->name << "  old LRStart stage: " << tblStg.second);
        }
    }

    // MinSTage status after updating slice liveranges and merged table minStage
    LOG7("MIN STAGE DEPARSER stage: " << self.self.phv.getDeparserStage());
    LOG7(PhvInfo::reportMinStages());
    LOG7("DG DEPARSER stage: " << (self.self.deps.max_min_stage + 1));
    LOG7(self.self.deps);
}

bool MergeAlwaysRunActions::UpdateAffectedTableMinStage::preorder(const IR::MAU::Table *tbl) {
    if (!PhvInfo::hasMinStageEntry(tbl) || !self.mergedARAwitNewStage)
        return false;

    revisit_visited();

    int oldMinStage = *(PhvInfo::minStages(tbl).begin());
    int newMinStage = self.self.deps.min_stage(tbl);

    tableMinStageShifts[tbl] = std::make_pair(oldMinStage, newMinStage);
    LOG4("\t\tMerge-affected table:" << tbl->name << " old minStage: " << oldMinStage <<
         " --> new minStage: " << newMinStage);
    return true;
}

// Go through IR Expression's to identify slices that need their liverange updated
// ---
bool MergeAlwaysRunActions::UpdateAffectedTableMinStage::preorder(const IR::Expression *t_expr) {
    if (!self.mergedARAwitNewStage)
        return false;

    auto *tbl = findContext<IR::MAU::Table>();
    if (tbl == nullptr) {
        LOG4("\t\t\t" << tableMinStageShifts.count(tbl) << " tables found for Expression "
             << *t_expr);
        return false;
    }
    bool is_write = isWrite();

    LOG4("  ---> Visiting "
         << (is_write ? "written" : "") <<" expr " << *t_expr);

    revisit_visited();

    LOG4("\t  - wrt table: " << tbl->name);

    // 1. Get field and bitrange
    le_bitrange bits;
    PHV::Field* exp_f = self.self.phv.field(t_expr, &bits);
    if (!exp_f) return true;

    LOG7("\t  - range " << bits << "  " << *exp_f);

    int oldStg = tableMinStageShifts.at(tbl).first;
    int newStg = tableMinStageShifts.at(tbl).second;
    auto deparsStg = PhvInfo::getDeparserStage();
    // PHV::FieldUse(PHV::FieldUse::WRITE));
    LOG7("\t  - oldStg " << oldStg);
    LOG7("\t  - newStg " << newStg);
    LOG7("\t  - oldDeparseStg " << deparsStg);

    // 2. Iterate over AllocSlices
    for (auto & sl : exp_f->get_alloc()) {
        if (!(sl.field_slice().overlaps(bits)))
            continue;

        bool modifyStart = false;
        bool modifyEnd = false;
        bool modifyDepars = false;
        bool singlePointLR = (sl.getEarliestLiveness() == sl.getLatestLiveness());
        int mergOldStg = oldStg;
        bool mergeAffectedSlice = false;
        bool no_tbl_LRstart_overlap = false;
        bool no_tbl_LRend_overlap = false;
        bool no_tbl_LR_overlap = false;

        LOG7("\t  - minLR " << sl.getEarliestLiveness());
        LOG7("\t  - maxLR " << sl.getLatestLiveness());

        // Check if AllocSlice LR has been affected by table merge
        PHV::StageAndAccess LRstart = sl.getEarliestLiveness();
        PHV::StageAndAccess LRend  = sl.getLatestLiveness();

        // Mark if slice's liverange is modified due to table merging;
        // if it is, update the slice liverange and the table old
        // stage to the pre-merge values (to avoid overlapping of the
        // liveranges of different allocated slices corresponding to
        // the same field slice.
        if (self.premergeLRstart.count(&sl)) {
            mergeAffectedSlice = true;
            if (self.premergeLRstart[&sl].count(tbl)) {
                LRstart.first = mergOldStg = self.premergeLRstart[&sl][tbl];
                LOG7("\t  - premerge LRstart oldStg " <<  mergOldStg);
            } else {
                no_tbl_LRstart_overlap = true;
            }
        }
        if (self.premergeLRend.count(&sl)) {
            mergeAffectedSlice = true;
            if (self.premergeLRend[&sl].count(tbl)) {
                LRend.first = mergOldStg =  self.premergeLRend[&sl][tbl];
                LOG7("\t  - premerge LRend oldStg " <<  mergOldStg);
            } else {
                no_tbl_LRend_overlap = true;
            }
        }

        // For allocated slices of field slices  that are not
        // originally accessed by merged tables set no_tbl_LR_overlap.
        // This helps bypass the liverange extension used to handle (non merged)
        // intermediate tables accessing field slices.
        if (!mergeAffectedSlice) {
            for (auto slTblStg : self.premergeLRstart) {
                if (slTblStg.second.count(tbl)) {
                    no_tbl_LR_overlap = true;
                }
            }
            for (auto slTblStg : self.premergeLRend) {
                if (slTblStg.second.count(tbl)) {
                    no_tbl_LR_overlap = true;
                }
            }
        }

        // Check first if maxStage is set to Deparser and mark for update
        // Then check if min/max stage matches oldStg and mark accordingly
        if ((deparsStg != (self.self.deps.max_min_stage + 1)) &&
            LRend.first == deparsStg &&
            (LRend.second.isWrite() || exp_f->deparsed())) {
            LOG7("\tUpdate last (deparser) stage from " << sl.getLatestLiveness() << " to " <<
                 (self.self.deps.max_min_stage+1) << " for slice: " << sl);
            modifyDepars = true;
        }

        if (!no_tbl_LR_overlap) {
            if (is_write) {
                if ((LRend.first == mergOldStg) &&
                    LRend.second.isWrite() &&
                    !tbl->is_always_run_action()) {
                    LOG7("\tUpdate last stage from " << mergOldStg << " to " << newStg <<
                         " for WRITTEN slice: " << sl);
                    modifyEnd = true;
                }

                if (singlePointLR && modifyEnd) {
                    LOG4("\tNot modifying LR start (same as end) for AllocSlice: " << sl);
                } else {
                    if (!no_tbl_LRstart_overlap &&
                        (LRstart.first == mergOldStg) &&
                        LRstart.second.isWrite()) {
                        LOG7("\tUpdate first stage from " << mergOldStg << " to " << newStg <<
                             " for WRITTEN slice: " << sl);
                        modifyStart = true;
                    }
                }

            } else {
                if (!no_tbl_LRend_overlap &&
                    (LRend.first == mergOldStg) &&
                    LRend.second.isRead()) {
                    LOG7("\tUpdate last stage from " << mergOldStg << " to " << newStg <<
                         " for  READ slice: " << sl);
                    modifyEnd = true;
                }

                if (singlePointLR && modifyEnd) {
                    LOG4("\tNot modifying LR start (same as end) for AllocSlice: " << sl);
                } else {
                    if ((LRstart.first == mergOldStg) &&
                        LRstart.second.isRead() &&
                        !tbl->is_always_run_action()) {
                        LOG7("\tUpdate first stage from " << mergOldStg << " to " << newStg <<
                             " for READ slice: " << sl);
                        modifyStart = true;
                    }
                }
            }
        }

        // Store liverange updates in order to apply all of them later
        auto sl_LR = std::make_pair(LRstart.first, LRend.first);
        auto sl_mod = std::make_pair(false, false);
        bool prvStart = false;
        bool prvEnd = false;

        if (sliceLRmodifies.count(&sl)) {
            sl_mod = sliceLRmodifies.at(&sl);

            if (sl_mod.first) {
                sl_LR.first = sliceLRshifts.at(&sl).first;
                prvStart = true;
            }
            if (sl_mod.second) {
                sl_LR.second = sliceLRshifts.at(&sl).second;
                prvEnd = true;
            }
        }

        if (modifyStart) {
            if (!prvStart) {
                sl_LR.first = newStg;
                sl_mod.first = true;
            } else if (sl_LR.first > newStg) {
                // extend LR if previous update was due to table
                // access in the middle of the LR
                sl_LR.first = newStg;
                sl_mod.first = true;
            }
        }

        if (modifyDepars) {
            sl_LR.second = (self.self.deps.max_min_stage + 1);
            sl_mod.second = true;
        } else if (modifyEnd) {
            if (!prvEnd) {
                sl_LR.second = newStg;
                sl_mod.second = true;
            } else if (sl_LR.second < newStg) {
                // extend LR if previous update was due to table
                // access in the middle of the LR
                sl_LR.second = newStg;
                sl_mod.second = true;
            }
        }

        sliceLRshifts[&sl] = sl_LR;
        sliceLRmodifies[&sl] = sl_mod;
        LOG7("   <--- Storing LR for " << sl << "  ===  [" << sl_LR.first << " : "
             << sl_LR.second << "]");
        LOG7("\t\t      mod: " <<  "  ===  [" << (sl_mod.first ? "vld" : " - ") <<
             " , " << (sl_mod.second ? "vld" : " - ") << "]");
    }  // for (auto & sl ...

    return false;
}

// Apply updates to table stages and slice liveranges
// ---
void MergeAlwaysRunActions::UpdateAffectedTableMinStage::end_apply() {
    if (!self.mergedARAwitNewStage)
        return;

    // Update the minStage of all tables affected by table merging
    // (stored in tableMinStageShifts)
    LOG4("\t Updating " << tableMinStageShifts.size() << " merge-affected tables:");

    for (auto entry : tableMinStageShifts) {
        int newStg = entry.second.second;

        PhvInfo::addMinStageEntry(entry.first, newStg, true);
    }

    // Update the liverange of all slices affected by table merging
    // (stored in sliceLRshifts)
    LOG4("\t Updating merge-affected slices:");

    for (auto entry : sliceLRshifts) {
        auto *sl = entry.first;

        LOG4("\t Modifying LR for slice " << *sl);

        sl->setLiveness(std::make_pair(entry.second.first, sl->getEarliestLiveness().second),
                        std::make_pair(entry.second.second, sl->getLatestLiveness().second));

        LOG4("\t\t ... to: " << sl->getEarliestLiveness() << " , " << sl->getLatestLiveness());
    }

    // Update the deparser-based liverange of slices not updated previously
    for (auto& fld : self.self.phv) {
        for (auto& sl : fld.get_alloc()) {
            if (sl.getLatestLiveness().first == self.self.phv.getDeparserStage()) {
                if (sliceLRshifts.count(&sl)) {
                    LOG5("\t WARNING: Modified LR still has old deparser LR stage for slice: " <<
                         sl);
                }
                sl.setLatestLiveness(
                    std::make_pair((self.self.deps.max_min_stage + 1),
                                   sl.getLatestLiveness().second));
            }
        }
    }

    // Update the deparser stage in minStages
    self.self.phv.setDeparserStage(self.self.deps.max_min_stage + 1);

    LOG7("MIN STAGE DEPARSER stage: " << self.self.phv.getDeparserStage());
    LOG7(PhvInfo::reportMinStages());
    LOG7("DG DEPARSER stage: " << (self.self.deps.max_min_stage + 1));
    LOG7(self.self.deps);
}

std::multimap<cstring, const TablePlacement::Placed *>::const_iterator
TablePlacement::find_placed(cstring name) const {
    auto rv = table_placed.find(name);
    if (rv == table_placed.end()) {
        if (auto p = name.findlast('.'))
            rv = table_placed.find(name.before(p));
        if (auto p = name.findlast('$'))
            rv = table_placed.find(name.before(p));
    }
    return rv;
}

void dump(const ordered_set<const DecidePlacement::GroupPlace *> &work) {
    std::cout << work << std::endl; }

TablePlacement::TablePlacement(const BFN_Options &opt, DependencyGraph &d,
                               const TablesMutuallyExclusive &m, PhvInfo &p,
                               LayoutChoices &l, const SharedIndirectAttachedAnalysis &s,
                               SplitAttachedInfo &sia, TableSummary &summary_,
                               MauBacktracker &mau_backtracker)
    : options(opt), deps(d), mutex(m), phv(p), lc(l), siaa(s), ddm(ntp, con_paths, d),
      att_info(sia), summary(summary_), mau_backtracker(mau_backtracker) {
    addPasses({
        &ntp,
        &con_paths,
        new SetupInfo(*this),
        new DecidePlacement(*this),
        new TransformTables(*this),
        // new MergeAlwaysRunActions(*this)
    });
}
