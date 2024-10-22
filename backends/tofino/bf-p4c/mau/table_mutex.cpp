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

#include "bf-p4c/mau/table_mutex.h"

#include "bf-p4c/lib/error_type.h"

bool IgnoreTableDeps::ignore_deps(const IR::MAU::Table *t1, const IR::MAU::Table *t2) const {
    auto t1_pos = ignore_dep_map.find(t1);
    if (t1_pos != ignore_dep_map.end()) {
        if (t1_pos->second.count(t2)) return true;
    }

    auto t2_pos = ignore_dep_map.find(t2);
    if (t2_pos != ignore_dep_map.end()) {
        if (t2_pos->second.count(t1)) return true;
    }
    return false;
}

bool IgnoreTableDeps::preorder(const IR::MAU::Table *tbl) {
    internal_name_to_table[tbl->name] = tbl;
    external_name_to_table[tbl->externalName()] = tbl;

    std::vector<IR::ID> annotation;
    tbl->getAnnotation("ignore_table_dependency"_cs, annotation);
    for (auto name : annotation) {
        // Due to P4_14 global name space, a dot is added to the initial table name
        table_to_pragmas[tbl].insert(name);
    }
    return true;
}

void IgnoreTableDeps::end_apply() {
    for (auto entry : table_to_pragmas) {
        const IR::MAU::Table *tbl = entry.first;
        for (auto pragma_val : entry.second) {
            const IR::MAU::Table *ign_tbl = nullptr;
            if (internal_name_to_table.count(pragma_val)) {
                ign_tbl = internal_name_to_table.at(pragma_val);
            } else if (external_name_to_table.count(pragma_val)) {
                // FIXME: For p4-16, honoring the dependency actually forces one of the
                // tables to fit, due to table_seqdeps not understanding the dependency
                // is gone corresponding to bad chain lengths.  Fix this after 9.0
                ign_tbl = external_name_to_table.at(pragma_val);
            } else if (external_name_to_table.count("."_cs + pragma_val)) {
                ign_tbl = external_name_to_table.at("." + pragma_val);
            } else {
                warning(BFN::ErrorType::WARN_PRAGMA_USE,
                        "%1%: The ignore_table_dependency "
                        "value %2% on table %3% does not have a corresponding backend match",
                        tbl, pragma_val, tbl->externalName());
                continue;
            }
            ignore_dep_map[tbl].insert(ign_tbl);
            ignore_dep_map[ign_tbl].insert(tbl);
        }
    }
}

safe_vector<IgnoreTableDeps::TablePair> IgnoreTableDeps::pairwise_deps_to_ignore() const {
    safe_vector<TablePair> rv;
    for (auto entry : ignore_dep_map) {
        auto first_tbl = entry.first;
        for (auto second_tbl : entry.second) {
            rv.emplace_back(std::make_pair(first_tbl, second_tbl));
        }
    }
    return rv;
}

bool TablesMutuallyExclusive::miss_mutex_action_chain(const IR::MAU::Table *tbl,
                                                      const IR::MAU::Action *default_act,
                                                      cstring &name) {
    ordered_set<cstring> non_def_act_chains;
    for (auto &n : tbl->next) {
        if (default_act->name.originalName == n.first) {
            name = n.first;
            return true;
        } else if (n.first[0] != '$') {
            non_def_act_chains.insert(n.first);
        }
    }

    if (non_def_act_chains.size() != tbl->actions.size() - 1) return false;
    if (tbl->has_default_path()) name = "$default"_cs;
    return true;
}

/**
 * The functionality of this algorithm:
 *     - All Tables in a TableSeq are not mutually exclusive with each other
 *     - The successors of these tables are also not mutually exclusive with each other
 *     - A table is not mutually exclusive with any of its successors
 * The reverse of the non-mutex graph is the mutual exclusion
 */
void TablesMutuallyExclusive::postorder(const IR::MAU::Table *tbl) {
    // Compute the table_succ entry for this table.
    bitvec succ;
    for (auto &n : tbl->next) {
        bitvec local_succ;
        for (auto t : n.second->tables) {
            succ |= table_succ[t];
            // Collect all the succcessors of tbl which will be affect the mutex analysis
            // due to exit table t
            local_succ.setbit(table_ids.at(t));
            exit_succ[tbl] |= exit_succ[t];
            if (t->is_exit_table()) {
                exit_succ[tbl] |= local_succ;
                break;
            }
        }
    }
    succ.setbit(table_ids.at(tbl));
    table_succ[tbl] = succ;

    // Update the non_mutex entry for this table to account for the successors we just computed.
    non_mutex[table_ids.at(tbl)] |= succ;
}

void TablesMutuallyExclusive::postorder(const IR::MAU::TableSeq *seq) {
    /* Update non_mutex to account for join points in the control flow. For example,
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
     * Here, we ensure that t4 is marked as not mutually exclusive with all of t1's table_succ
     * entries.*/
    for (size_t i = 0; i < seq->tables.size(); i++) {
        auto i_tbl = seq->tables.at(i);
        if (i_tbl->is_exit_table()) continue;
        for (size_t j = i + 1; j < seq->tables.size(); j++) {
            auto j_tbl = seq->tables.at(j);
            for (auto i_id : table_succ[i_tbl]) {
                // Ensure that if the one of the successor of i_tbl is an exit table, then
                // some of the table_succ of i_tbl will be mutually exclusive with j_tbl and its
                // sucessors
                if (exit_succ.count(i_tbl) && exit_succ[i_tbl].getbit(i_id)) continue;
                non_mutex[i_id] |= table_succ[j_tbl];
            }
            if (j_tbl->is_exit_table()) break;
        }
    }
}

bool TablesMutuallyExclusive::operator()(const IR::MAU::Table *a, const IR::MAU::Table *b) const {
    BUG_CHECK(table_ids.count(a), "No table info for %1%", a->externalName());
    BUG_CHECK(table_ids.count(b), "No table info for %1%", b->externalName());
    return !non_mutex(table_ids.at(a), table_ids.at(b));
}

std::vector<const IR::MAU::Action *> SharedIndirectAttachedAnalysis::get_indirect_actions(
    const IR::MAU::Table *a, const IR::MAU::AttachedMemory *am) {
    std::vector<const IR::MAU::Action *> attached_actions;
    if (am->is<IR::MAU::ActionData>() || am->is<IR::MAU::Selector>()) {
        safe_vector<ActionData::Format::Use> action_format_vec;
        if (auto res = a->resources) {
            action_format_vec.push_back(res->action_format);
        } else {
            action_format_vec = lc.get_action_formats(a);
        }
        for (auto action_format : action_format_vec) {
            for (auto act : Values(a->actions)) {
                if (action_format.if_action_has_action_data(act->name)) {
                    attached_actions.push_back(act);
                }
            }
        }
    } else if (am->is<IR::MAU::Counter>() || am->is<IR::MAU::Meter>() ||
               am->is<IR::MAU::StatefulAlu>()) {
        for (auto act : Values(a->actions)) {
            if (act->stateful_call(am->name)) {
                attached_actions.push_back(act);
            }
        }
    }
    return attached_actions;
}

bool SharedIndirectAttachedAnalysis::check_if_can_share(const IR::MAU::Table *a,
                                                        const IR::MAU::Table *b,
                                                        const IR::MAU::AttachedMemory *am) {
    if (mutex(a, b)) {
        return true;
    } else if (ignore.ignore_deps(a, b)) {
        bitvec am_tbl_bv;
        am_tbl_bv.setbit(table_ids.at(b));
        _mutex_through_ignore[table_ids.at(a)] |= am_tbl_bv;
    } else {
        for (auto act_a : get_indirect_actions(a, am)) {
            for (auto act_b : get_indirect_actions(b, am)) {
                if (!action_mutex(act_a, act_b)) return false;
            }
        }
    }
    return true;
}

bool SharedIndirectAttachedAnalysis::preorder(const IR::MAU::AttachedMemory *am) {
    visitAgain();
    if (am->direct) return false;
    auto *tbl = findContext<IR::MAU::Table>();
    for (auto am_tbl : backend_users[am]) {
        if (tbl == am_tbl) continue;
        if (check_if_can_share(tbl, am_tbl, am)) {
            table_sharing_attached[tbl].insert(am_tbl);
            continue;
            // Stateful Register can be shared across actions
        } else if (!am->to<IR::MAU::StatefulAlu>()) {
            error(
                "%1% and %2% cannot share %3% because use of the %3% is not "
                "mutually exclusive",
                tbl, am_tbl, am);
        }
    }
    backend_users[am].push_back(tbl);
    return false;
}

// for gtest
bool SharedIndirectAttachedAnalysis::if_table_share_attach(const IR::MAU::Table *a,
                                                           const IR::MAU::Table *b) const {
    if (table_sharing_attached.count(a)) {
        if (table_sharing_attached.at(a).count(b)) return true;
    }
    if (table_sharing_attached.count(b)) {
        if (table_sharing_attached.at(b).count(a)) return true;
    }
    return false;
}

bool SharedIndirectAttachedAnalysis ::mutex_through_ignore(const IR::MAU::Table *a,
                                                           const IR::MAU::Table *b) const {
    BUG_CHECK(table_ids.count(a), "No table info for %1%", a->externalName());
    BUG_CHECK(table_ids.count(b), "No table info for %1%", b->externalName());
    return _mutex_through_ignore(table_ids.at(a), table_ids.at(b));
}
