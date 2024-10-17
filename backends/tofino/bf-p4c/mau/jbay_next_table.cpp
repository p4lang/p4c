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

#include "jbay_next_table.h"
#include <unordered_map>
#include <unordered_set>
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/table_layout.h"
#include "device.h"
#include "lib/error.h"
#include "bf-p4c/common/table_printer.h"


/* This pass calculates 2 data structures used during assembly generation:
 *   1. ~props~ specifies a set of tables to be run after a given table is applied and a certain
 *      condition is met. This specifies how the next table "signal" propagtes through tables at
 *      runtime.  More specifically, for a given table and a given table sequence owned by that
 *      table, it specifies the set of table that are to run under that table sequence's
 *      condition.   The JbayNextTable object captures one table that needs to be run and how that
 *      signal is propagate to it (either by LOCAL_EXEC, GLOBAL_EXEC, or LONG_BRANCH).  The pass
 *      is split into 3 stages: EmptyIds, NextTableAlloc, and AddDummyTables (which must run in that
 *      order).
 *   2. ~lbs~ specifies the long branch tags that a source table needs to set and which tables need
 *      to be associated with each tag. This is necessary to generate the correct ASM, although
 *      effectively captures similar information in props, just in a format that is more usable in
 *      assembly generation.
 *
 * AT A GLANCE:
 *
 * 1. Prop calculates the ~props~ map. It does so according to push-down, as described in the
 *    optimizations section. Additionally, it collects a lot of data that is used throughout the
 *    rest of the pass. In building the ~props~ map, it builds the information needed to run
 *    LBAlloc, the next pass. Furthermore, it finds which tables are to be set to always run, which
 *    is then added by TagReduce.
 *
 * 2. LBAlloc doesn't really need to be a pass, as it does not actually iterate over the graph. In
 *    the init_apply, it takes the data captured by Prop and allocates all of the long branch uses
 *    into tags.
 *
 * 3. TagReduce is responsible for two things: (1) setting always run on tables; and (2) reducing
 *    the number of long branch tags to a level that is supported by the device. It accomplishes (2)
 *    by adding dumb tables, as described below. If it is able to merge tags, it sets Prop and
 *    LBAlloc to run again.
 *
 * Passes are run in order: Prop, LBAlloc, TagReduce, Prop (if needed), LBAlloc (if needed). Prop
 * and LBAlloc are only repeated if TagReduce reduced the number of tags.
 *
 * OPTIMIZATIONS:
 *
 * This pass attempts to minimize the use of long branches wherever possible, as LOCAL_EXEC and
 * GLOBAL_EXEC are effectively unlimited. This is accomplished via push-down, which achieves a
 * minimal usage of long branches for a given table placement. Unoptimized NTP would use a long
 * branch across an entire table sequence if *any* of the tables in the sequence were placed 2
 * stages or later. The following example will be used to illustrate these optimizations:
 *
 *                    t3       t5     t7
 *              t  t1 t2       t4     t6  t8        TABLES
 * -----------------------------------------------
 *  0  1  2  3  4  5  6  7  8  9  10  11  12 (...)  STAGES
 *              |                          |
 *         first_stage                 last_stage
 *
 * where t is the parent table and t1-t8 are the tables in one of its table sequences (e.g. $miss).
 *
 * 1. Next table push-down: Tables in a table sequence may be made responsible for propagating next
 *    table signals from their parent. Unoptimized NTP would run a long branch from table t in stage
 *    4 to stage 11, setting t1 through t8 to run off that long branch. With push-down, NTP becomes
 *    as follows:
 *
 *    +----+----+---------------------+
 *    | 05 | t1 | global_exec from t  |
 *    +----+----+---------------------+
 *    | 06 | t2 | global_exec from t1 |
 *    | 06 | t3 | local_exec from t2  |
 *    +----+----+---------------------+
 *    | 09 | t4 | long_branch from t2 |
 *    | 09 | t5 | local_exec from t4  |
 *    +----+----+---------------------+
 *    | 11 | t6 | long_branch from t4 |
 *    | 11 | t7 | local_exec from t6  |
 *    +----+----+---------------------+
 *    | 12 | t8 | global_exec from t6 |
 *    +----+----+---------------------+
 *
 *    This limits long_branch usage to stage 6 to 9 and 9 to 11. Notice how our parent table t is
 *    now only responsible for propagating the signal to t1, which then propagates the signal to t2,
 *    etc. This functionality is accomplished by adding a $run_if_ran "branch" (a sort of fake table
 *    sequence). The $run_if_ran branch for a table t is a set of tables that should be run whenever
 *    t is run. During assembly generation, tables in $run_if_ran are added to every other table
 *    sequence.
 *
 * 2. Tag allocation: Once we have our uses, we must allocate them into tags. Tags can be used for
 *    multiple long branches as long as the live ranges do not overlap. If the uses are on different
 *    gresses, there is an additional restriction that there must be a 1 stage gap between the two
 *    uses, to account for the different timings of the gresses. See below for a clarifying example.
 *
 *       0  1  2  3  4  5  6  7  8  9
 *    0  |--------|-----|             <--- Tightest merge when two uses are on same gress
 *    1  |========|  |--------|       <--- Tightest merge when two uses are on different gresses
 *                ^^^^ Single stage bubble for timing
 *
 * 3. Add dumb tables: Inject tables that do nothing more than propagate a global exec signal. This
 *    is the most aggressive optimization, often capable of entirely eliminating long branches. When
 *    a stage that has no tables from this table sequence is also not completely full, we inject a
 *    dumb table into it and use global exec instead. For sake of argument, suppose that stages 7
 *    and 10 are not full (unused logical IDs) but stage 8 is full. The resulting NTP is as follows:
 *
 *    +----+----+---------------------+
 *    | 05 | t1 | global_exec from t  |
 *    +----+----+---------------------+
 *    | 06 | t2 | global_exec from t1 |
 *    | 06 | t3 | local_exec from t2  |
 *    +----+----+---------------------+
 *    | 07 | d1 | global_exec from t2 | <--- new dumb table added!
 *    +----+----+---------------------+
 *    | 09 | t4 | long_branch from d1 | <--- long branch comes from d1, not t2!
 *    | 09 | t5 | local_exec from t4  |
 *    +----+----+---------------------+
 *    | 10 | d2 | global_exec from t4 | <--- new dumb table added!
 *    +----+----+---------------------+
 *    | 11 | t6 | global_exec from d2 | <--- global exec from d2 instead of long branch from t2
 *    | 11 | t7 | local_exec from t6  |
 *    +----+----+---------------------+
 *    | 12 | t8 | global_exec from t6 |
 *    +----+----+---------------------+
 *
 *    Note how in this version we got rid of the second long branch entirely and were able to
 *    shorten the first long branch by 1 stage. However, this optimization comes at a cost of adding
 *    logical IDs, which may be a precious resource. As such, the TagReduce algorithm only performs
 *    dumb table injection when the number of tags has exceeded the device limits. Additionally, it
 *    attempts to find an injection that uses the fewest number of tables possible. See merging
 *    methods in TagReduce for full algorithm.
 *
 */



/**
 * This is the first pass that is designed to deal with the new Table Placement IR restrictions,
 * specifically the ones that have arisen because of the Tofino2 placement algorithm.
 * Previously, it was a guarantee that a table would appear in a single TableSeq
 * object.  However, this is no longer the case, specifically after table placement.  Let's delve
 * into why for this example.
 *
 * The program looked like the following:
 *
 * apply {
 *     if (condition) {
 *         t1.apply();
 *         switch(t2.apply().action_run()) {
 *             a1 : {}
 *             a2 : { t3.apply(); t4.apply(); }
 *         }
 *     }
 * }
 *
 * In table placement, the condition was being combined with table t2.  In order to maintain the
 * correct IR structure, t1 must now be applied on all of t2's actions, including possibly a
 * new default action choice.
 *
 * In Tofino this would be fine as the IR could look something like this:
 *
 * apply {
 *     switch(condition && t2.apply().action_run()) {
 *         a1 : { t1.apply(); }
 *         a2 : {
 *            t3.apply();
 *            switch(t4.apply().action_run()) {
 *                default : { t1.apply(); }
 *            }
 *         }
 *         default : { t1.apply(); }
 *     }
 * }
 *
 * This is because when t2 is placed, both t3 and t4 would be guaranteed to be placed before t1 is
 * placed, as next table propagation arises from a single table.
 *
 * However in Tofino2 and onwards, due to the next table propagation expanding, the following
 * placement is allowed.
 *
 * apply {
 *     switch(condition && t2.apply().action_run()) {
 *         a1 : { t1.apply(); }
 *         a2 : { t3.apply(); t1.apply(); t4.apply(); }
 *         default : { t1.apply(); }
 *     }
 *
 * This is because the next table propagation can completely interleave tables inside and outside
 * of control blocks.  The ordering of tables in both of these programs comes from their logical
 * ids, and t1's logical id can now be interleaved anywhere from the beginning of t3 to the end of
 * t4.
 *
 * This obviously breaks the IR implicit restriction that tables only appear in a single table
 * sequence.  Furthermore this breaks the default next assumption, that each table has a single
 * default next table.  This is fine to break in theory, as these should not be the restrictions.
 * This leads me to propose three IR guarantees for tables that should be the rules (after
 * table placement and eventually before table placement).
 *
 * 1.  Each table apply statement should be mutually exclusive, specifically a table should
 *     not be applied more than one time a packet.  This one is fairly obvious why.  A table
 *     can only be applied once by the hardware.
 *
 * 2.  The control flow off of a table must be the same in each pathway.  Thus the following
 *     program is not possible:
 *
 *         if (t0.apply().hit) {
 *             switch (t1.apply().action_run) {
 *                 a1 : { t2.apply(); }
 *                 a2 : { t3.apply(); }
 *             }
 *         } else {
 *             switch (t1.apply().action_run) {
 *                 a1 : { t3.apply(); }
 *                 a2 : { t2.apply(); }
 *             }
 *         }
 *
 *     In the Tofino HW, the entry holds the next table to run, and is static.   This cannot be
 *     changed by which pathway reached the table.
 *
 * 3.  The tables must have a topological order.  Thus the following program is invalid.
 *
 *         switch (t1.apply().action_run) {
 *             a1 : { t2.apply(); t3.apply(); }
 *             a2 : { t3.apply(); t2.apply(); }
 *         }
 *
 *     This is because the tables themselves have a logical order through predication.  This
 *     would also make the following program impossible:
 *
 *         switch (t1.apply().action_run) {
 *             a1 : { t2.apply(); t3.apply(); }
 *             a2 : { t3.apply(); t4.apply(); }
 *             a3 : { t4.apply(); t2.apply(); }
 *         }
 *
 * 4.  In Tofino only, the default next table must be the same on all pathways.  In Tofino, not
 *     Tofino2, this program is impossible:
 *
 *         switch (t1.apply().action_run) {
 *             a1 : { t2.apply(); }
 *             a2 : { t2.apply(); t3.apply(); }
 *         }
 *
 *     The default next table out of t2 would not be valid.
 *
 * Passes can be run that can guarantee these properties.  Right now, guaranteeing them after
 * table placement only in Tofino2 is sufficient.
 */

/**
 * The purpose of this pass is to determine which tables always precede another table in
 * table placement.  Let's take a look at an example,
 *
 * apply {
 *     switch (t1.apply().action_run) {
 *         a1 : { t3.apply(); }
 *         a2 : { t2.apply(); t3.apply(); t4.apply(); }
 *     }
 * }
 *
 * In this example, in order to lower the propagation down to the lowest level, one can note
 * that t2 always precedes t3 and t4, but that t3 does not always precede t4.  Thus it is not
 * safe to execute t4 from when t3 is running.  Let say the allocation is the following:
 *
 *  t1    t2    t3    t4   TABLES
 * ----------------------
 *  0  1  2  3  4  5  6    STAGES
 *
 * In the original case now two long branches would be required, a long branch that runs from
 * set by t1 and read by t3, and another long branch set by t1 and read by t2 and t4.  With
 * the localizing of the long branches, now instead of propagating a long branch between t1 and t4,
 * t2 can set a long branch directly to t4 and reuse the long branch to t3.  This localization,
 * while it does not reduce long branch requirements, does reduce dummy table requirements in
 * order to merge.
 */
bool JbayNextTable::LocalizeSeqs::BuildTableToSeqs::preorder(const IR::MAU::Table *tbl) {
    auto seq = findContext<IR::MAU::TableSeq>();
    self.table_to_seqs[tbl].insert(seq);
    return true;
}

bool JbayNextTable::LocalizeSeqs::BuildCanLocalizeMaps::preorder(const IR::MAU::Table *tbl) {
    if (tbl->always_run == IR::MAU::AlwaysRun::ACTION) return true;
    bool first_run = true;
    ordered_set<const IR::MAU::Table *> seen_after;
    for (auto seq : self.table_to_seqs.at(tbl)) {
        bool before_table = true;
        ordered_set<const IR::MAU::Table *> local_seen_after;
        for (auto seq_tbl : seq->tables) {
            if (seq_tbl == tbl) {
                before_table = false;
                continue;
            }
            if (before_table) continue;
            local_seen_after.insert(seq_tbl);
        }

        BUG_CHECK(!before_table, "BuildTableToSeqs is broekn");
        if (first_run) {
            seen_after = local_seen_after;
        } else {
            seen_after &= local_seen_after;
        }
        first_run = false;
    }
    self.can_localize_prop_to[tbl] = seen_after;

    LOG4("\t    LocalizeSequences : " << tbl->externalName());
    for (auto t_a : seen_after) {
        LOG4("\t\tAfter table " << t_a->externalName());
    }
    return true;
}

class JbayNextTable::FindNextTableUse : public MauTableInspector {
    JbayNextTable &self;
    std::vector<const IR::MAU::Table *> tables;

    int first_id(const IR::MAU::TableSeq *seq) {
        for (auto *t : seq->tables) {
            if (t->always_run != IR::MAU::AlwaysRun::ACTION)
                return *t->global_id(); }
        return -1; }
    int stage_diff(std::pair<int, int> range) {
        return range.second/Memories::LOGICAL_TABLES - range.first/Memories::LOGICAL_TABLES; }
    // a single table may be both the end of a range (be invoked by next table) and the
    // start of a range (invoke another table via next_table) wthout interference, so
    // equal end points does not count as an 'overlap'
    bool overlaps(std::pair<int, int> r1, std::pair<int, int> r2) {
        return r1.second > r2.first && r2.second > r1.first; }

    profile_t init_apply(const IR::Node *root) {
        self.use_next_table.clear();
        tables.clear();
        return MauTableInspector::init_apply(root); }
    void postorder(const IR::MAU::Table *t) override {
        // only care about tables that have control dependent successors (for now)
        // FIXME -- such tables could use next_table to trigger the next table from the
        // containing seq, if that was useful.  Probably almost never useful?
        if (!t->next.empty()) tables.push_back(t); }
    void postorder(const IR::BFN::Pipe *) {
        std::stable_sort(tables.begin(), tables.end(),
                         [this](const IR::MAU::Table *a, const IR::MAU::Table *b) {
            return self.control_dep.paths(a) > self.control_dep.paths(b); });
        for (auto *t : tables) {
            std::pair<int, int> range;
            range.first = range.second = *t->global_id();
            for (auto *seq : Values(t->next)) {
                range.second = std::max(range.second, first_id(seq)); }
            if (stage_diff(range) < 2 && self.control_dep.paths(t) <= 8) {
                // can use just local/global exec -- no need for next_table or long_branch
                continue; }
            bool can_use_next = true;
            for (auto &p : self.use_next_table) {
                if (p.first->gress != t->gress) continue;
                if (self.mutex(p.first, t)) continue;
                if (self.control_dep(p.first, t)) continue;
                if (self.control_dep(t, p.first)) continue;
                if (!overlaps(range, p.second)) continue;
                can_use_next = false;
                break; }
            if (can_use_next) {
                self.use_next_table.emplace(t, range);
            } else if (self.control_dep.paths(t) > 8) {
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%s requires too many next table "
                      "encodings for indirect map and can't fit within it", t);
            }
        }
    }

 public:
    explicit FindNextTableUse(JbayNextTable &s) : self(s) {}
};

/* Holds next table prop information for a specific table sequence. One created per call to
 * add_table_seq. Note that add_table_seq may be called on the same table sequence multiple times,
 * as table sequences can both be (1) under the same table multiple times (multiple actions run the
 * same tables); (2) under different tables. In the case of (1), we do not need to run it twice, but
 * do so right now out of convenience. In the case of (2), we do need to run it multiple times,
 * since the NTP needs to be associated with different tables. */
struct JbayNextTable::Prop::NTInfo {
    const IR::MAU::Table* parent;  // Origin of this control flow
    dyn_vector<dyn_vector<const IR::MAU::Table*>> stages;  // Tables in each stage
    const int first_stage;
    int last_stage;
    const cstring seq_nm;  // Name of the table sequence
    const IR::MAU::TableSeq* ts;

    NTInfo(const IR::MAU::Table* tbl, std::pair<cstring, const IR::MAU::TableSeq*> seq)
            : parent(tbl), first_stage(tbl->stage()), seq_nm(seq.first), ts(seq.second) {
        LOG4("NTP for " << tbl->name << " and sequence " << seq.first << ":");
        last_stage = tbl->stage();
        BUG_CHECK(first_stage >= 0, "Unplaced table %s", tbl->name);

        // Collect tables into stages for this sequence
        for (auto t : ts->tables) {
            auto st = t->stage();
            if (t->is_always_run_action()) continue;
            if (t->is_detached_attached_tbl) continue;
            BUG_CHECK(t->global_id(), "Unplaced table %s", t->name);
            BUG_CHECK(tbl->global_id(), "Unplaced table %s", tbl->name);
            BUG_CHECK(first_stage <= st, "Table %s (LID: %d) placed before parent %s (LID: %d)",
                      t->name, *t->global_id(), tbl->name, *tbl->global_id());
            // Update last stage
            last_stage = st > last_stage ? st : last_stage;
            // Add to the correct vector
            stages[st].push_back(t);
        }
    }
};

std::pair<ssize_t, ssize_t> JbayNextTable::get_live_range_for_lb_with_dest(UniqueId dest) const {
  for (auto some_lbus_use : lbus) {
    if (some_lbus_use.dest && some_lbus_use.dest->unique_id() == dest)
      return std::make_pair(some_lbus_use.fst, some_lbus_use.lst);
  }
  return std::make_pair(-1, -1);
}

// Checks if there is any overlap between two tags
bool JbayNextTable::LBUse::operator&(const LBUse& r) const {
    BUG_CHECK(fst <= lst && r.fst <= r.lst, "LBUse first and last have been corrupted!");
    if ((lst - fst) < 2 || (r.lst - r.fst) < 2)
        return false;
    else if (thread() == r.thread())
        return !(fst >= r.lst || lst <= r.fst);
    else
        return !(fst > r.lst || lst < r.fst);
}

void JbayNextTable::LBUse::extend(const IR::MAU::Table* t) {
    ssize_t st = t->stage();
    BUG_CHECK(ssize_t(st) < lst, "Table %s trying to long branch to earlier stage!", t->name);
    // If tables are in the same stage, we don't need to do anything
    if (st == fst) return;
    fst = st < fst ? st : fst;
}

bool JbayNextTable::Tag::add_use(const LBUse& lbu) {
    for (auto u : uses)
        if (u & lbu) return false;
    uses.push_back(lbu);
    return true;
}

JbayNextTable::profile_t JbayNextTable::Prop::init_apply(const IR::Node* root) {
    auto rv = MauInspector::init_apply(root);
    // Clear maps, since we have to rebuild them
    self.props.clear();
    self.stage_id.clear();
    self.lbus.clear();
    self.dest_src.clear();
    self.dest_ts.clear();
    self.al_runs.clear();
    self.max_stage = 0;
    LOG1("BEGINNING NEXT TABLE PROPAGATION");
    return rv;
}

void JbayNextTable::Prop::local_prop(const NTInfo &nti, std::map<int, bitvec> &executed_paths) {
    if (size_t(nti.first_stage) >= nti.stages.size()) return;
    // Guarantee that all tables no matter what are preceded by the parent control table
    for (auto nt : nti.stages.at(nti.first_stage)) {
        BUG_CHECK(nti.parent->global_id(), "Unplaced table %s", nti.parent->name);
        BUG_CHECK(nt->global_id(), "Unplaced table %s", nt->name);
        BUG_CHECK(*nti.parent->global_id() <= *nt->global_id(),
                  "Table %s has LID %d, less than parent table %s (LID %d)", nt->name,
                  *nt->global_id(), nti.parent->name, *nti.parent->global_id());
        LOG3("  - " << nt->name << " to " << nti.seq_nm << "; LOCAL_EXEC from " << nti.parent->name
             << " in stage " << nt->stage());
        self.props[get_uid(nti.parent)][nti.seq_nm].insert(get_uid(nt));
        executed_paths[nti.first_stage].setbit(*nt->global_id());
    }
    // Vertically compress each stage; i.e. calculate all of the local_execs, giving us a single
    // representative table for each stage to use in cross stage propagation
    for (int i = nti.first_stage + 1; i <= nti.last_stage && size_t(i) < nti.stages.size(); ++i) {
        auto& tables = nti.stages.at(i);
        if (tables.empty()) continue;  // Skip empty stages

        // Tables should be in logical ID order. The "representative" table (lowest LID) for this
        // stage and its set of $run_if_ran tables
        bitvec local_exec_tables;
        for (size_t j = 0; j < tables.size(); j++) {
            auto rep = tables.at(j);
            auto& r_i_r = self.props[get_uid(rep)]["$run_if_ran"_cs];
            for (size_t k = j + 1; k < tables.size(); k++) {
                auto nt = tables.at(k);
                BUG_CHECK(rep->global_id(), "Unplaced table %s", rep->name);
                BUG_CHECK(nt->global_id(), "Unplaced table %s", nt->name);
                BUG_CHECK(*rep->global_id() < *nt->global_id(),
                          "Tables not in logical ID order; representative %s has greater LID "
                          "than %s.", rep->name, nt->name);
                if (local_exec_tables.getbit(k)) continue;
                // In order to be local exec by another table, must be propagated to
                if (self.localize_seqs.can_propagate_to(rep, nt)) {
                    LOG3("  - " << nt->name << " on $run_if_ran; LOCAL_EXEC from " << rep->name
                         << " in stage " << nt->stage());
                    r_i_r.insert(get_uid(nt));
                    local_exec_tables.setbit(k);
                    executed_paths[i].setbit(*nt->global_id());
                }
            }
        }
    }
}

void JbayNextTable::Prop::cross_prop(const NTInfo &nti, std::map<int, bitvec> &executed_paths) {
    auto stages = nti.stages;
    for (int i = nti.first_stage + 1; i <= nti.last_stage; i++) {
        if (stages[i].empty()) continue;
        for (auto rep : stages.at(i)) {
            if (executed_paths[i].getbit(*rep->global_id())) continue;
            cstring branch = "$run_if_ran"_cs;
            int prev_st = 0;
            const IR::MAU::Table *prev_t = nullptr;
            for (int j = i - 1; j >= nti.first_stage; j--) {
                for (auto pt : stages.at(j)) {
                    if (!self.localize_seqs.can_propagate_to(pt, rep))
                        continue;
                    prev_t = pt;
                    prev_st = j;
                    break;
                }
                if (prev_t != nullptr)
                    break;
            }
            if (prev_t == nullptr) {
                prev_t = nti.parent;
                branch = nti.seq_nm;
                prev_st = nti.parent->stage();
            }

            // Logging
            if (prev_st == i-1) {  // Global exec
                LOG3("  - " << rep->name << " on " << branch
                     << "; GLOBAL_EXEC from " << prev_t->name << " on range [" << prev_st << " -- "
                     << i << "]");
            } else {  // Long branch
                BUG_CHECK(prev_t->stage() + 1 < rep->stage(),
                          "LB used between %s in stage %d and %s in stage %d!",
                          prev_t->name, prev_t->stage(), rep->name, rep->stage());
                LOG3("  - " << rep->name << " on " << branch
                     << "; LONG_BRANCH from " << prev_t->name << " on range [" << prev_st << " -- "
                     << i << "]");

                // If we already have an lb for this dest, extend it
                if (self.lbus.count(LBUse(rep))) {
                    auto it = self.lbus.find(LBUse(rep));
                    auto lb = *it;
                    self.lbus.erase(it);
                    lb.extend(prev_t);
                    self.lbus.insert(lb);
                } else {  // Otherwise, create a new LBUse
                    self.lbus.insert(LBUse(rep, prev_st, i));
                }
                self.dest_src[get_uid(rep)].insert(get_uid(prev_t));
                self.dest_ts[get_uid(rep)].insert(nti.ts);
            }
            // Add to the propagation map for asm gen
            self.props[get_uid(prev_t)][branch].insert(get_uid(rep));
            executed_paths[i].setbit(*rep->global_id());
        }
    }
}

void JbayNextTable::Prop::next_table_prop(const NTInfo &nti,
                                          std::map<int, bitvec> &executed_paths) {
    auto stages = nti.stages;
    for (int i = nti.first_stage; i <= nti.last_stage; i++) {
        if (stages[i].empty()) continue;
        auto *first = stages[i].front();
        if (i - nti.first_stage < 2 && self.control_dep.paths(nti.parent) < 8)
            break;
        LOG3("  - " << first->name << " on " << nti.seq_nm << "; NEXT_TABLE from " <<
             nti.parent->name << " on range [" << nti.first_stage << " -- " << i << "]");
        executed_paths[i].setbit(*first->global_id());
        self.props[get_uid(nti.parent)][nti.seq_nm].insert(get_uid(first));
        break; }
}

bool JbayNextTable::Prop::preorder(const IR::MAU::Table* t) {
    if (t->always_run == IR::MAU::AlwaysRun::ACTION)
        return true;
    int st = t->stage();
    self.max_stage = st > self.max_stage ? st : self.max_stage;
    if (!self.mems[st]) self.mems[st].reset(Memories::create());
    self.mems[st]->update(t->resources->memuse);
    if (*t->global_id() > static_cast<int>(self.stage_id[st]))
        self.stage_id[st] = *t->global_id();
    if (!findContext<IR::MAU::Table>())
        self.al_runs.insert(t->unique_id());
    // Add all of the table's table sequences
    LOG4("    Prop preorder Table " << t->externalName());
    for (auto ts : t->next) {
        std::map<int, bitvec> executed_paths;
        NTInfo nti(t, ts);
        if (self.use_next_table.count(t)) {
            next_table_prop(nti, executed_paths); }
        local_prop(nti, executed_paths);
        cross_prop(nti, executed_paths);
    }
    return true;
}

void JbayNextTable::Prop::end_apply() {
    for (int i = 0; i < self.max_stage; ++i) {  // Fix up stage_id
        if (self.stage_id[i] < i * Memories::LOGICAL_TABLES)
            self.stage_id[i] = i * Memories::LOGICAL_TABLES;
        else
            self.stage_id[i]++;  // Collected max, but need next open one
    }
    LOG1("FINISHED NEXT TABLE PROPAGATION");
}

JbayNextTable::profile_t JbayNextTable::LBAlloc::init_apply(const IR::Node* root) {
    auto rv = MauInspector::init_apply(root);  // Early exit
    // Clear old values
    self.stage_tags.clear();
    self.lbs.clear();
    self.max_tag = -1;
    self.use_tags.clear();
    LOG1("BEGIN LONG BRANCH TAG ALLOCATION");
    // Get LBU's in order of their first stage, for better merging
    std::vector<LBUse> lbus(self.lbus.begin(), self.lbus.end());
    std::sort(lbus.begin(), lbus.end(), [](const LBUse& l, const LBUse& r)
                                            { return (l.fst < r.fst); });
    // Loop through all of the tags in order of their first stage
    for (auto& u : lbus) {
        int tag = alloc_lb(u);
        LOG3("  Long branch targeting " << u.dest->name << " allocated on tag " << tag);
        // Loop through all sources and mark them in the map we expose
        for (auto src : self.dest_src[get_uid(u.dest)]) {
            LOG5("    - source: " << src.build_name());
            self.lbs[src][tag].insert(get_uid(u.dest));
        }
    }
    LOG1("FINISHED LONG BRANCH TAG ALLOCATION");
    return rv;
}

int JbayNextTable::LBAlloc::alloc_lb(const LBUse& u) {
    int tag = 0;
    bool need_new = true;
    // Keep looping and trying to add LB to a tag
    for (; size_t(tag) < self.stage_tags.size(); ++tag) {
        if (self.stage_tags[tag].add_use(u)) {
            need_new = false;
            break;
        }
    }
    if (need_new) {
        tag = self.stage_tags.size();
        Tag t;
        t.add_use(u);
        self.stage_tags.push_back(t);
        self.max_tag = tag;
    }
    self.use_tags[u] = tag;
    return tag;
}

bool JbayNextTable::LBAlloc::preorder(const IR::BFN::Pipe*) {
    return false;
}

// Pretty prints tag information
void JbayNextTable::LBAlloc::pretty_tags() {
    std::vector<std::string> header;
    header.push_back("Tag #");
    for (int i = 0; i < self.max_stage; ++i)
        header.push_back("Stage " + std::to_string(i));

    std::map<gress_t, std::string> gress_char;
    gress_char[INGRESS] = "-";
    gress_char[EGRESS] = "+";
    gress_char[GHOST] = "=";
    log << "======KEY======" << std::endl
        << "INGRESS: " << gress_char[INGRESS] << std::endl
        << "EGRESS:  " << gress_char[EGRESS] << std::endl
        << "GHOST:   " << gress_char[GHOST] << std::endl;

    TablePrinter* tp = new TablePrinter(log, header, TablePrinter::Align::CENTER);
    tp->addSep();

    // Print each tag's occupied stages
    int tag_num = 0;
    for (auto t : self.stage_tags) {
        std::vector<std::string> row;
        row.push_back(std::to_string(tag_num));
        std::map<int, std::string> stage_occ;
        for (auto u : t) {  // Iterate through all of the uses
            if (u.fst >= u.lst - 1) continue;  // skip reduced uses
            std::string spacer = gress_char[u.dest->thread()];
            stage_occ[u.fst] += "|" + spacer;
            for (ssize_t i = u.fst + 1; i < u.lst; ++i) {
                stage_occ[i] = spacer + spacer;
            }
            stage_occ[u.lst] = spacer + ">" + stage_occ[u.lst];
        }
        for (int i = 0; i < self.max_stage; ++i) {
            if (stage_occ.count(i))
                row.push_back(stage_occ[i]);
            else
                row.push_back("");
        }
        tp->addRow(row);
        ++tag_num;
    }
    tp->print();
    log << std::endl;
}

void JbayNextTable::LBAlloc::pretty_srcs() {
    std::vector<std::string> header({"Dest. table", "Tag #", "Range", "Src tables"});
    TablePrinter* tp = new TablePrinter(log, header, TablePrinter::Align::CENTER);
    tp->addSep();

    for (auto kv : self.use_tags) {
        std::vector<std::string> row;
        row.push_back(get_uid(kv.first.dest).build_name());
        row.push_back(std::to_string(kv.second));
        row.push_back("[ " + std::to_string(kv.first.fst) + " -- " + std::to_string(kv.first.lst)
                      + " ]");
        std::string srcs("[ ");
        for (auto s : self.dest_src[get_uid(kv.first.dest)])
            srcs += s.build_name() + ", ";
        srcs.pop_back();  // Delete last space ...
        srcs.pop_back();  // and comma
        srcs += " ]";
        row.push_back(srcs);
        tp->addRow(row);
    }
    tp->print();
    log << std::endl;
}

void JbayNextTable::LBAlloc::end_apply() {
    pretty_tags();
    LOG2(log.str());
    // Reset and print source dest info
    log.clear();
    pretty_srcs();
    LOG3(log.str());
}

JbayNextTable::profile_t JbayNextTable::TagReduce::init_apply(const IR::Node* root) {
    auto rv = MauTransform::init_apply(root);
    stage_dts.clear();
    dumb_tbls.clear();
    return rv;
}

IR::Node* JbayNextTable::TagReduce::preorder(IR::BFN::Pipe* p) {
    if (self.stage_tags.size() <= size_t(Device::numLongBranchTags())) {
        return p;
    }
    // Try to merge tags
    LOG1("BEGINNING TAG REDUCTION");
    bool success = merge_tags();
    if (!success) {
        LOG1("TAG REDUCTION FAILED! BACKTRACKING AND RETRYING WITHOUT LONG BRANCHES!");
        // Backtrack, because we can't add DTs to fix the problem!
        throw LongBranchAllocFailed();
        prune();
        return p;
    }
    // Allocate memories for DTs
    alloc_dt_mems();
    LOG1("FINISHED TAG REDUCTION SUCCESSFULLY, INSERTING TABLES INTO IR");
    return p;
}

IR::Node* JbayNextTable::TagReduce::preorder(IR::MAU::TableSeq* ts) {
    // Get the original pointer
    auto key = dynamic_cast<const IR::MAU::TableSeq*>(getOriginal());
    // Add new tables

    auto control_tbl = findContext<IR::MAU::Table>();
    if (control_tbl == nullptr)
        return ts;

    if (dumb_tbls[key].empty())
        return ts;

    // Only add tables that are after the table starting this particular part of the sequence
    for (auto dumb_tbl : dumb_tbls[key]) {
        if (*dumb_tbl->global_id() > *control_tbl->global_id()) {
            ts->tables.push_back(dumb_tbl);
        }
    }

    // Put tables into LID sorted order
    std::sort(ts->tables.begin(), ts->tables.end(),
              [](const IR::MAU::Table* t1, const IR::MAU::Table* t2)
              { return *t1->global_id() < *t2->global_id(); });
    return ts;
}

IR::Node* JbayNextTable::TagReduce::preorder(IR::MAU::Table* t) {
    if (t->always_run == IR::MAU::AlwaysRun::ACTION)
        return t;
    if (self.al_runs.count(t->unique_id()))
        t->always_run = IR::MAU::AlwaysRun::TABLE;
    return t;
}

/* Represents a symmetric matrix as a flattened vector. (i, j) = (j, i) */
template <class T>
class JbayNextTable::TagReduce::sym_matrix {
    std::vector<T> m;
    size_t dim;
    inline bool inrng(size_t i) const { return i < dim; }
    // Converts 2D indexing to 1D
    inline size_t conv(size_t i, size_t j) const {
        if (!inrng(i) || !inrng(j))
            throw std::out_of_range("Index out of range in tag matrix!");
        size_t x = std::max(i, j);
        size_t y = std::min(i, j);
        return x * (x + 1) / 2 + y;
    }
    // Takes an index in the 1D vector and converts back to a coordinate pair. Always returns in
    // order (smaller, larger)
    static std::pair<size_t, size_t> invert(size_t z) {
        size_t x = floor((sqrt(double(8*z + 1)) - 1.0)/2.0);
        size_t y = z - (x * (x + 1)) / 2;
        return std::pair<size_t, size_t>(y, x);
    }

 public:
    explicit sym_matrix(size_t num) : m((num*(num+1))/2, T()), dim(num) {}
    // Lookup (i,j)
    T operator()(size_t i, size_t j) const {
        return m.at(conv(i, j));
    }
    // Assign vec to (i,j), returns old value
    T operator()(size_t i, size_t j, T val) {
        T old = m[conv(i, j)];
        m[conv(i, j)] = val;
        return old;
    }
    // Returns index of an object given a comparison function. comp should return true if the left
    // argument is to be preferred. Returns index in order (smaller, larger)
    std::pair<size_t, size_t> get(std::function<bool(T, T)> comp) const {
        size_t idx = 0;
        T best = m.at(0);
        for (size_t i = 1; i < m.size(); ++i) {
            if (comp(m.at(i), best)) {
                idx = i;
                best = m.at(i);
            }
        }
        return invert(idx);
    }
};

struct JbayNextTable::TagReduce::merge_t {
    Tag merged;
    std::map<LBUse, std::set<int>> dum;
    bool success;
    bool finalized;
    size_t num_dts;
    merge_t() : success(true), finalized(false), num_dts(0) {}
};

JbayNextTable::TagReduce::merge_t JbayNextTable::TagReduce::merge(Tag l, Tag r) const {
    merge_t rv;
    std::map<int, int> stage_id(self.stage_id);  // Need our own copy
    auto cap = [&](int k) {  // Check the capacity of a stage. 0 means full
                   return (k+1) * Memories::LOGICAL_TABLES - stage_id[k];
               };
    auto addrng = [&](int fst, int lst) {  // Returns a set containing [fst, lst)
                      std::set<int> dts;
                      for (; fst < lst; ++fst) {
                          if (!cap(fst)) rv.success = false;
                          dts.insert(fst);
                          rv.num_dts++;
                      }
                      return dts;
                  };
    for (auto& lu : l) {
        for (auto& ru : r) {
            if (!(lu & ru)) continue;  // Skip if there's no overlap
            std::set<int> dts;
            LBUse* key;  // The LB which dummy tables were added to
            // Handles when one tag fully covers another
            auto overlap = [&](LBUse* lrg, LBUse* sml) {
                               if (true) {  // Try to get rid of smaller
                                   dts = addrng(sml->fst + 1, sml->lst);
                                   sml->fst = sml->lst;
                                   key = sml;
                               } else {  // Get rid of larger o.w.
                                   dts = addrng(lrg->fst + 1, lrg->lst);
                                   lrg->fst = lrg->lst;
                                   key = lrg;
                               }
                           };
            // Handles when one tag only partially covers another
            auto partial
                = [&](LBUse* lft, LBUse* rgt) {
                      // Here, we can add rf--ll-1 to the left or rf+1--ll if on. If off, we can add
                      // max(lf + 1, rf - 1)--ll-1 to the left or rf+1--min(ll + 1, rl - 1) to the
                      // right
                      bool on = lft->same_gress(*rgt);
                      int lbeg = on ? lft->fst : std::max(lft->fst + 1, rgt->fst - 1);
                      int rend = on ? lft->lst : std::min(lft->lst + 1, rgt->lst - 1);
                      if (cap(lbeg) > cap(rend)) {
                          dts = addrng(lbeg, lft->lst);
                          lft->lst = lbeg;
                          key = lft;
                      } else {
                          dts = addrng(rgt->fst + 1, rend + 1);
                          rgt->fst = rend;
                          key = rgt;
                      }
                  };
            // Four ways to conflict:
            if (lu.fst <= ru.fst && ru.lst <= lu.lst)  // lu completely overlaps ru
                overlap(&lu, &ru);
            else if (ru.fst <= lu.fst && lu.lst <= ru.lst)  // ru completely overlaps lu
                overlap(&ru, &lu);
            else if (lu.fst < ru.fst && lu.lst < ru.lst)  // lu partially overlaps ru
                partial(&lu, &ru);
            else  // ru partially overlaps lu
                partial(&ru, &lu);
            if (!rv.success) return rv;
            BUG_CHECK(dts.size() > 0, "Two uses overlap but don't need dummy tables to merge??");
            // Insert into map, update the key
            for (auto i : dts) {
                rv.dum[*key].insert(i);
                stage_id[i]++;
            }
        }
    }
    // Merge tags
    for (auto u : l)
        rv.merged.add_use(u);
    for (auto u : r)
        rv.merged.add_use(u);
    rv.finalized = true;
    return rv;
}

// Only called after we determine we need dumb tables. Finds (locally) minimal configuration of dumb
// tables
JbayNextTable::TagReduce::sym_matrix<JbayNextTable::TagReduce::merge_t>
JbayNextTable::TagReduce::find_merges() const {
    // Comparison between tag i and tag j
    sym_matrix<merge_t> m(self.stage_tags.size());
    // Get all of the possible dummy tables
    for (size_t i = 0; i < self.stage_tags.size(); ++i) {
        // Compare to tags greater than i (comparisons are commutative
        for (size_t j = i + 1; j < self.stage_tags.size(); ++j)
            m(i, j, merge(self.stage_tags.at(i), self.stage_tags.at(j)));
    }
    return m;
}

// Merges tags until we've reduced long branch pressure far enough. Returns true if LB pressure can
// be lowered to below device limtis, false o.w.
bool JbayNextTable::TagReduce::merge_tags() {
    // Continue merging until our long branches have been minimized
    for (int num_merges = self.stage_tags.size() - Device::numLongBranchTags();
         num_merges; --num_merges) {
        auto m = find_merges();  // Find the merges we can do on this iteration
        // Get the coordinates of the smallest vector that is not empty
        size_t fst, snd;
        std::tie(fst, snd) = m.get([&](merge_t l, merge_t r) {
                                       if (!l.success || !l.finalized) return false;
                                       if (!r.success || !r.finalized) return true;
                                       return l.num_dts < r.num_dts;
                                   });
        merge_t mrg = m(fst, snd);  // Dumb tables needed to merge fst and snd
        if (!mrg.success || !mrg.finalized) {  // If we can't find a successful merge, fail
            return false;
        }
        LOG2("  Merging tags " << fst << " and " << snd);
        for (auto kv : mrg.dum) {  // Associate all of the dummy tables with the table sequence
            auto dtbls =
                std::accumulate(kv.second.begin(), kv.second.end(), std::vector<IR::MAU::Table *>(),
                                [&](std::vector<IR::MAU::Table *> a, int st) {
                                    cstring tname = "$next-table-forward-to-"_cs +
                                                    kv.first.dest->name + "-" + std::to_string(st);
                                    LOG3("    - " << tname << " in stage " << st
                                         << ", targeting dest " << kv.first.dest->name);
                                    auto* dt = new IR::MAU::Table(tname, kv.first.thread());
                                    dt->set_global_id(self.stage_id[st]++);
                                    stage_dts[st].push_back(dt);
                                    a.push_back(dt);
                                    return a;
                                });
            for (auto ts : self.dest_ts[get_uid(kv.first.dest)])
                dumb_tbls[ts].insert(dumb_tbls[ts].end(), dtbls.begin(), dtbls.end());
        }
        // Delete the old tags and add the merged tag
        self.stage_tags[fst] = mrg.merged;
        self.stage_tags.erase(self.stage_tags.begin() + snd);
    }
    return true;
}

void JbayNextTable::TagReduce::alloc_dt_mems() {
    // Allocate memories for dumb tables
    for (auto stage : stage_dts) {
        // Need to store resources until allocation has finished
        std::map<IR::MAU::Table*, TableResourceAlloc*> tras;
        auto& mem = self.mems[stage.first];
        if (!mem) mem.reset(Memories::create());
        for (auto* t : stage.second) {
            TableResourceAlloc* tra = new TableResourceAlloc;
            tras[t] = tra;
            mem->add_table(t, nullptr, tra, nullptr, nullptr,
                           ActionData::FormatType_t::default_for_table(t), 0, 0, {});
            self.num_dts++;
        }
        bool success = mem->allocate_all_dummies();
        // Just to check that allocation succeeded. If a logical ID is available (which we check
        // when allocating dummy tables), we should be allocate a dummy table since there are 16 gws
        // NOTE: Chris says that this can actually be done with an always miss logical table. May
        // want to change this later to not allocate a gateway!
        BUG_CHECK(success, "Could not allocate all dummy tables in stage %d!", stage.first);
        // Set the resources
        for (auto res : tras)
            res.first->resources = res.second;
    }
}

JbayNextTable::JbayNextTable(bool disableNextTableUse) {
    addPasses({ &localize_seqs,
                &mutex,
                &control_dep,
                disableNextTableUse ? nullptr : new FindNextTableUse(*this),
                new Prop(*this),
                new LBAlloc(*this),
                new TagReduce(*this) });
}

ordered_set<UniqueId> JbayNextTable::next_for(const IR::MAU::Table *tbl, cstring what) const {
    if (what == "$miss" && tbl->next.count("$try_next_stage"_cs))
        what = "$try_next_stage"_cs;

    if (tbl->actions.count(what) && tbl->actions.at(what)->exitAction) {
        BUG("long branch incompatible with exit action");
        return {};
    }
    if (!tbl->next.count(what))
        what = "$default"_cs;
    ordered_set<UniqueId> rv;
    if (props.count(tbl->unique_id())) {
        auto prop = props.at(tbl->unique_id());
        // We want to build this set according to the NextTableProp
        // Add tables according to next table propagation (if it has an entry in the map)
        for (auto id : prop[what])
            rv.insert(id);
        // Always add run_if_ran set
        for (auto always : prop["$run_if_ran"_cs])
            rv.insert(always);
    }
    return rv;
}
