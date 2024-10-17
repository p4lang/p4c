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

#include "bf-p4c/mau/action_mutex.h"

/**
 * The functionality of this algorithm, pretty much the exact same algorithm used right now
 * in table mutex:
 *
 *     - All Tables in a TableSeq are not mutually exclusive with each other, meaning that
 *       all of their actions are not mutually exclusive with each other.
 *     - The successors of each of these tables in a TableSeq would also not be mutually
 *       exclusive with each other either, thus their actions would not be either
 *     - A table can have multiple branches, and the any actions that run in order to start
 *       that branch are not mutually exclusive with any tables on that branch.
 */
void ActionMutuallyExclusive::postorder(const IR::MAU::Table *tbl) {
    bitvec all_actions_in_table;
    for (const auto *act : Values(tbl->actions))
        all_actions_in_table.setbit(action_ids[act]);

    std::map<cstring, bitvec> actions_running_on_branch;

    // Determine which actions are going to run on which branches, and include those in
    // that particular branch.
    // A table is not an action_chain if it has one and only one path ($default).  Comes when
    // one wants to force a control dependency in the program
    if (tbl->action_chain() || tbl->has_default_path()) {
        for (const auto *act : Values(tbl->actions)) {
            if (tbl->next.count(act->name.originalName) > 0) {
                actions_running_on_branch[act->name.originalName].setbit(action_ids[act]);
            } else if (tbl->has_default_path()) {
                actions_running_on_branch["$default"_cs].setbit(action_ids[act]);
            }
        }
    } else if (tbl->hit_miss_p4()) {
        for (const auto *act : Values(tbl->actions)) {
            if (tbl->next.count("$hit"_cs) > 0 && !act->miss_only())
                actions_running_on_branch["$hit"_cs].setbit(action_ids[act]);
            if (tbl->next.count("$miss"_cs) > 0 && !act->hit_only()) {
                actions_running_on_branch["$miss"_cs].setbit(action_ids[act]);
            }
        }
    }

    bitvec all_succ = all_actions_in_table;
    for (auto next_table_seq_kv : tbl->next) {
        cstring branch_name = next_table_seq_kv.first;
        auto next_table_seq = next_table_seq_kv.second;
        bitvec local_succ;
        for (auto next_table : next_table_seq->tables)
            local_succ |= action_succ[next_table];
        // All actions that begin a particular branch would not be mutually exclusive with tables
        // on that branch
        for (auto i : actions_running_on_branch[branch_name])
            not_mutex[i] |= local_succ;
        all_succ |= local_succ;
    }

    action_succ[tbl] = all_succ;
}

/**
 * Stolen comments from table_mutex, but it is the same baseline algorithm:
 *
 * Update non_mutex to account for join points in the control flow. For example,
 *
 *   switch (t1.apply().action_run) {
 *     a1: { t2.apply(); }
 *     a2: { t3.apply(); }
 *   }
 *   t4.apply();
 * 
 * is represented by
 * 
 *   [ t1  t4 ]
 *    /  \
 * [t2]  [t3]
 * 
 * Here, we ensure that t4's actions are marked as not mutually exclusive with all of actions of
 * each of the entries in t1's table_succ.
 */
void ActionMutuallyExclusive::postorder(const IR::MAU::TableSeq *seq) {
    for (size_t i = 0; i < seq->tables.size(); i++) {
        auto i_tbl = seq->tables.at(i);
        for (size_t j = i+1; j < seq->tables.size(); j++) {
            auto j_tbl = seq->tables.at(j);
            for (auto i_id : action_succ[i_tbl])
                not_mutex[i_id] |= action_succ[j_tbl];
        }
    }
}
