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

#include "bf-p4c/mau/dynamic_dep_metrics.h"

void DynamicDependencyMetrics::score_on_seq(const IR::MAU::TableSeq *seq, const IR::MAU::Table *tbl,
                                            int &max_dep_impact, char type) const {
    for (auto seq_tbl : seq->tables) {
        LOG3("\t" << type << " table " << seq_tbl->externalName());
        if (seq_tbl == tbl) return;
    }

    for (auto seq_tbl : seq->tables) {
        if (placed_tables(seq_tbl)) continue;
        int curr_impact = dg.dependence_tail_size_control_anti(seq_tbl);
        curr_impact += dg.stage_info.at(tbl).dep_stages_dom_frontier;
        LOG2("    " << type << " impact : " << seq_tbl->externalName() << " " << curr_impact);
        max_dep_impact = std::max(max_dep_impact, curr_impact);
    }
}

/**
 * So in Tofino specifically, this is to help place tables in sequences due to the other tables.
 * Let's look at the following example:
 *
 * apply() {
 *     t1.apply();
 *     if (t2.apply().hit) {
 *         t3.apply();
 *     }
 *     t4.apply();
 * }
 *
 * The sequence nodes in this case will be:
 *
 *   [ t1, t2, t4 ]
 *          |
 *          | $hit
 *          |
 *          V
 *        [ t3 ]
 *
 * Now I'm going to assign dependency chains, though let's say that these have no direct
 * dependencies with each other:
 *
 *     t1 - 7
 *     t2 - 9
 *     t3 - 0
 *     t4 - 8
 *
 * When we place table t2, t3 must be placed before any other tables hit.  However, t3 will
 * not be favored directly.  But in this case, because t4 is waiting, the metrics for t3
 * must see this function.
 *
 * The purpose of this function is to compare two tables, specifically the different tables
 * that they affect until they get placed, and get the max dependency chain from all tables
 * that this particular table is waiting on.  This is called a downward propagation score
 * as we are propagating dependency information down from a control flow dominator.
 *
 * TODO: Eventually instead of the top TableSequence, all table sequences leading up to that table
 * should be analyzedj
 *
 * This constraint is completely voided in Tofino2
 */
std::pair<int, int> DynamicDependencyMetrics::get_downward_prop_score(
    const IR::MAU::Table *a, const IR::MAU::Table *b) const {
    ordered_set<const IR::MAU::Table *> a_tables;
    ordered_set<const IR::MAU::Table *> b_tables;
    int a_max_dep_impact = 0;
    int b_max_dep_impact = 0;

    for (safe_vector<const IR::Node *> a_path : con_paths.table_pathways.at(a)) {
        for (safe_vector<const IR::Node *> b_path : con_paths.table_pathways.at(b)) {
            if (std::find(a_path.begin(), a_path.end(), b) != a_path.end()) continue;
            if (std::find(b_path.begin(), b_path.end(), a) != b_path.end()) continue;

            auto a_path_it = a_path.rbegin();
            auto b_path_it = b_path.rbegin();
            while (a_path_it != a_path.rend() && b_path_it != b_path.rend()) {
                if (*a_path_it != *b_path_it) break;
                a_path_it++;
                b_path_it++;
            }

            while (a_path_it != a_path.rend()) {
                auto a_seq = (*a_path_it)->to<IR::MAU::TableSeq>();
                if (a_seq != nullptr) {
                    score_on_seq(a_seq, a, a_max_dep_impact, 'A');
                }
                a_path_it++;
            }

            while (b_path_it != b_path.rend()) {
                auto b_seq = (*b_path_it)->to<IR::MAU::TableSeq>();
                if (b_seq != nullptr) {
                    score_on_seq(b_seq, b, b_max_dep_impact, 'B');
                }
                b_path_it++;
            }
        }
    }

    a_max_dep_impact = std::max(a_max_dep_impact, dg.dependence_tail_size_control_anti(a));
    b_max_dep_impact = std::max(b_max_dep_impact, dg.dependence_tail_size_control_anti(b));

    LOG2("    A table " << a->externalName() << " " << dg.dependence_tail_size_control_anti(a)
                        << " " << dg.stage_info.at(a).dep_stages_dom_frontier);
    LOG2("    B table " << b->externalName() << " " << dg.dependence_tail_size_control_anti(b)
                        << " " << dg.stage_info.at(b).dep_stages_dom_frontier);
    return std::make_pair(a_max_dep_impact, b_max_dep_impact);
}

/**
 * Based on how many dependencies are within the control dominating set.
 */
int DynamicDependencyMetrics::total_deps_of_dom_frontier(const IR::MAU::Table *a) const {
    int rv = 0;
    for (auto tbl : ntp.control_dom_set.at(a)) {
        rv += dg.happens_before_dependences(tbl).size();
    }
    return rv;
}

/**
 * Given what has already been placed within a stage, return if the table, and any table that the
 * next table has to be propagated into can be placed in this stage due to dependencies.  Relevant
 * for Tofino.
 */
bool DynamicDependencyMetrics::can_place_cds_in_stage(
    const IR::MAU::Table *tbl, ordered_set<const IR::MAU::Table *> &already_placed_in_stage) const {
    if (dg.stage_info.at(tbl).dep_stages_dom_frontier > 0) return false;
    for (auto cd_tbl : ntp.control_dom_set.at(tbl)) {
        for (auto comp_tbl : already_placed_in_stage) {
            if (dg.happens_phys_before(comp_tbl, cd_tbl)) return false;
        }
    }
    return true;
}

/**
 * Given what has already been placed within a stage, return the number of tables that can be placed
 * of this table next table dominating set
 */
int DynamicDependencyMetrics::placeable_cds_count(
    const IR::MAU::Table *tbl, ordered_set<const IR::MAU::Table *> &already_placed_in_stage) const {
    int rv = 0;
    for (auto cd_tbl : ntp.control_dom_set.at(tbl)) {
        bool can_fit = true;
        for (auto comp_tbl : ntp.control_dom_set.at(tbl)) {
            if (cd_tbl == comp_tbl) continue;
            if (!dg.happens_phys_before(comp_tbl, cd_tbl)) continue;
            can_fit = false;
            break;
        }

        for (auto comp_tbl : already_placed_in_stage) {
            if (!dg.happens_phys_before(comp_tbl, cd_tbl)) continue;
            can_fit = false;
            break;
        }
        if (!can_fit) continue;
        rv++;
    }
    return rv;
}

/**
 * Return the average chain length of every table in the control dominating set
 */
double DynamicDependencyMetrics::average_cds_chain_length(const IR::MAU::Table *tbl) const {
    double sum = 0;
    for (auto cds_tbl : ntp.control_dom_set.at(tbl)) {
        sum += dg.happens_before_dependences(cds_tbl).size();
    }
    return sum / ntp.control_dom_set.at(tbl).size();
}
