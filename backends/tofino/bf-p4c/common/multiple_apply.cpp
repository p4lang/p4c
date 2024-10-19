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

#include "multiple_apply.h"

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/mau/default_next.h"
#include "bf-p4c/mau/table_flow_graph.h"

Visitor::profile_t MultipleApply::MutuallyExclusiveApplies::init_apply(const IR::Node *root) {
    mutex_apply.clear();
    return MauInspector::init_apply(root);
}

void MultipleApply::MutuallyExclusiveApplies::postorder(const IR::MAU::Table *tbl) {
    // Ensure the table being visited was derived from an actual table in the P4 source.
    if (tbl->match_table == nullptr) return;

    if (mutex_apply.count(tbl->match_table)) {
        // We've previously encountered other applications of this table. Check that this
        // application is mutually exclusive with all those other applications.
        for (auto other_table : mutex_apply.at(tbl->match_table)) {
            if (!mutex(tbl, other_table)) {
                error("%s: Not all applies of table %s are mutually exclusive", tbl->srcInfo,
                      tbl->externalName());
                errors.emplace(tbl->match_table->externalName());
            }
        }
    }

    // Add this application to the set.
    mutex_apply[tbl->match_table].emplace(tbl);
}

Visitor::profile_t MultipleApply::CheckStaticNextTable::init_apply(const IR::Node *root) {
    auto result = MauInspector::init_apply(root);
    self.duplicate_tables.clear();
    canon_table.clear();
    all_gateways.clear();
    return result;
}

/**
 *
 * The program to support looks something like this:
 *
 *   if (A.apply().hit) {
 *       if (condition-1) {
 *           B.apply();
 *           if (condition-2)
 *               C.apply();
 *       } else if (condition-2) {
 *           C.apply();
 *       }
 *   } else {
 *       if (condition-3)
 *           B.apply();
 *       if (condition-2)
 *           C.apply();
 *   }
 *
 * The conditions here are equivalent.  However, similar to standard match tables, out of
 * extract_maupipe, all of the individual tables are unique.
 *
 * In order for the program to be supportable on Tofino, B's default next table must the same
 * on all applications.  In this case, B's default_next would be condition-2, which if all
 * condition-2 gateways are the same IR node, then this is supportable.  Without this function,
 * the algorithm only verifies that match tables that have identical static control flow.
 * Previously, all applications of condition-2 will never be combined.
 *
 * This currently will mark any two gateways as duplicates, if their condition is equivalent, and
 * their static control flow are equivalent.  The argument is that if a program is supportable
 * without merging two equivalent conditions, that program will still be supportable with all
 * equivalent conditions merged:
 *
 *     You cannot write a P4 program that is supportable on Tofino with non-merged equivalent
 *     conditions but not supportable with merged equivalent conditions.
 *
 * This is rooted from the default next constraint for Tofino.  Would it be possible to break
 * the default next constraint if this gateway was merged.  My argument is no:
 *
 * if (condition-equiv-1) { controlA.apply(); }
 * if (condition-equiv-2) { controlA.apply(); }
 *
 * Because the conditions are equivalent, the control flow under them must be equivalent.
 * The default next table of those separate control flows also must be equivalent (if it is a
 * valid P4 program).  By then de-duplicating the condition, this has no affect on the default
 * next able of controlA, meaning that the de-duplication was safe.
 */
void MultipleApply::CheckStaticNextTable::check_all_gws(const IR::MAU::Table *tbl) {
    // Do nothing if we've encountered tis gateway already
    if (all_gateways.count(tbl)) return;

    all_gateways.insert(tbl);
    for (auto gw : all_gateways) {
        // As Jed has noted, before marking sub-children as equivalent, verify if the gateways
        // themselves are equivalent
        if (gw != tbl && check_equiv(gw, tbl, false)) {
            // Found an equivalent gateway.  Mark it and its children as being equivalent
            check_equiv(gw, tbl);
            return;
        }
    }
}

void MultipleApply::CheckStaticNextTable::postorder(const IR::MAU::Table *tbl) {
    // Mark duplicate tables and check that tables that are supposed to be duplicates really are
    // equivalent.

    // Add current table to the union-find data structure.
    self.duplicate_tables.insert(tbl);

    // Only proceed if the current table corresponds to an actual table at the P4 source level.
    if (tbl->match_table == nullptr) {
        check_all_gws(tbl);
        return;
    }

    BUG_CHECK(!tbl->conditional_gateway_only(),
              "A Table object corresponding to a table at the P4 source level is unexpectedly "
              "gateway-only: %1%",
              tbl->externalName());

    if (canon_table.count(tbl->match_table) == 0) {
        // This is the first time we are encountering an application of this table. Nothing to
        // check, but record this as the canonical table.
        canon_table[tbl->match_table] = tbl;
        return;
    }

    // Check that the conditional control flow following this table application is the same as that
    // for the canonical Table object, and mark corresponding components of the IR subtrees rooted
    // at the two Tables as being duplicated.
    //
    // First, check that the next-table entries for the table being visited also appear as
    // next-table entries for the canonical table.
    auto canon_tbl = canon_table.at(tbl->match_table);
    for (auto &entry : tbl->next) {
        auto &key = entry.first;
        auto &cur_seq = entry.second;

        if (canon_tbl->next.count(key) == 0) {
            error(
                "Table %1% has incompatible next-table chains: not all applications of this "
                "table have a next-table chain for %2%.",
                tbl->externalName(), key);
            return;
        }

        auto canon_seq = canon_tbl->next.at(key);
        if (canon_seq->size() != cur_seq->size()) {
            error(
                "Table %1% has incompatible next-table chains: not all applications of this "
                "table have the same chain length for %2%.",
                tbl->externalName(), key);
            return;
        }

        for (size_t i = 0; i < cur_seq->size(); i++) {
            auto cur_seq_tbl = cur_seq->tables.at(i);
            auto canon_seq_tbl = canon_seq->tables.at(i);
            if (!check_equiv(cur_seq_tbl, canon_seq_tbl)) {
                error(
                    "Table %1% has incompatible next-table chains for %2%, differing at "
                    "position %3%, with tables %4% and %5%",
                    tbl->externalName(), key, i, cur_seq_tbl->externalName(),
                    canon_seq_tbl->externalName());
            }
        }
    }

    // Also check the reverse: the next-table entries for the canonical table also appear as
    // next-table entries for the table being visited.
    for (auto &entry : canon_tbl->next) {
        auto &key = entry.first;

        if (tbl->next.count(key) == 0) {
            error(
                "Table %1% has incompatible next-table chains: not all applications of this "
                "table have a next-table chain for %2%.",
                tbl->externalName(), key);
            return;
        }
    }

    // Mark the current table as being duplicated with the canonical table.
    self.duplicate_tables.makeUnion(tbl, canon_tbl);
}

bool MultipleApply::CheckStaticNextTable::check_equiv(const IR::MAU::Table *table1,
                                                      const IR::MAU::Table *table2,
                                                      bool makeUnion) {
    if (table1 == table2) return true;
    if (!table1 || !table2) return false;

    // Add the tables to the union-find data structure.
    self.duplicate_tables.insert(table1);
    self.duplicate_tables.insert(table2);

    // If the two tables have been marked as duplicates, then we have already checked their
    // equivalence.
    if (self.duplicate_tables.find(table1) == self.duplicate_tables.find(table2)) return true;

    if (table1->match_table != table2->match_table) return false;
    if (table1->conditional_gateway_only() != table2->conditional_gateway_only()) return false;

    if (table1->conditional_gateway_only()) {
        // Check gateway rows for equivalence.
        if (table1->gateway_rows.size() != table2->gateway_rows.size()) return false;

        for (size_t idx = 0; idx < table1->gateway_rows.size(); ++idx) {
            auto row1 = table1->gateway_rows.at(idx);
            auto row2 = table2->gateway_rows.at(idx);

            if (row1.second != row2.second) return false;
            if (!equiv_gateway(row1.first, row2.first)) return false;
        }
    }

    // Check next-table map.
    if (table1->next.size() != table2->next.size()) return false;
    for (auto entry1 : table1->next) {
        auto next_name = entry1.first;
        auto next_table1 = entry1.second;

        if (!table2->next.count(next_name)) return false;
        auto next_table2 = table2->next.at(next_name);

        if (!check_equiv(next_table1, next_table2, makeUnion)) return false;
    }

    // Mark the tables as duplicates.
    if (makeUnion) self.duplicate_tables.makeUnion(table1, table2);

    return true;
}

bool MultipleApply::CheckStaticNextTable::check_equiv(const IR::MAU::TableSeq *seq1,
                                                      const IR::MAU::TableSeq *seq2,
                                                      bool makeUnion) {
    if (seq1 == seq2) return true;
    if (!seq1 || !seq2) return false;

    if (seq1->tables.size() != seq2->tables.size()) return false;
    for (size_t idx = 0; idx < seq1->tables.size(); ++idx) {
        auto table1 = seq1->tables.at(idx);
        auto table2 = seq2->tables.at(idx);
        if (!check_equiv(table1, table2, makeUnion)) return false;
    }

    return true;
}

bool MultipleApply::CheckStaticNextTable::equiv_gateway(const IR::Expression *expr1,
                                                        const IR::Expression *expr2) {
    // This implements a simple check, which just determines whether the two expressions are
    // structurally equivalent, without considering semantic equivalence.
    if (expr1 == expr2) return true;
    if (!expr1 || !expr2) return false;
    return expr1->equiv(*expr2);
}

Visitor::profile_t MultipleApply::DeduplicateTables::init_apply(const IR::Node *root) {
    auto result = MauTransform::init_apply(root);
    replacements.clear();
    table_seqs_seen.clear();
    return result;
}

const IR::Node *MultipleApply::DeduplicateTables::postorder(IR::MAU::Table *tbl) {
    auto orig_tbl = getOriginal<IR::MAU::Table>();
    auto canon_tbl = self.duplicate_tables.find(orig_tbl);

    auto &replacements = this->replacements[getGress()];
    if (replacements.count(canon_tbl)) return replacements.at(canon_tbl);

    // This is the first time we are encountering this table. No replacement to make here, but if
    // tbl hasn't been changed, then return orig_tbl to make sure the visitor framework doesn't
    // throw away our result.
    const IR::MAU::Table *result = *tbl == *orig_tbl ? orig_tbl : tbl;
    replacements[canon_tbl] = result;
    return result;
}

const IR::Node *MultipleApply::DeduplicateTables::postorder(IR::MAU::TableSeq *seq) {
    auto &table_seqs_seen = this->table_seqs_seen[getGress()];
    for (auto seq_seen : table_seqs_seen) {
        if (*seq == *seq_seen) return seq_seen;
    }

    // This is the first time we are encountering this table sequence. No replacement to make here,
    // but if seq hasn't been changed, then return orig_seq to make sure the visitor framework
    // doesn't throw away our result.
    auto orig_seq = getOriginal<IR::MAU::TableSeq>();
    const IR::MAU::TableSeq *result = *seq == *orig_seq ? orig_seq : seq;
    table_seqs_seen.push_back(result);
    return result;
}

bool MultipleApply::CheckTopologicalTables::preorder(const IR::MAU::TableSeq *thread) {
    // Each top-level TableSeq should represent the whole MAU pipeline for a single gress. Build
    // the control-flow graph for that.
    FlowGraph graph;
    thread->apply(FindFlowGraph(graph));

    // Check for cycles in the control-flow graph.
    std::set<const IR::MAU::Table *> tables_reported;
    for (const auto &entry : graph.tableToVertex) {
        const auto *table = entry.first;

        // Ignore graph's sink node.
        if (table == nullptr) continue;

        // Don't check current table if we've already reported it as being involved in a cycle.
        if (tables_reported.count(table)) continue;

        if (!graph.can_reach(table, table)) continue;
        auto cycle = graph.find_path(table, table);

        // Build a string representation of the tables involved in the cycle. Lop off the last
        // element of the cycle, since it will be the same as the first.
        cycle.pop_back();
        std::stringstream out;
        bool first = true;
        for (auto tbl : cycle) {
            if (!first) out << ", ";

            auto name = tbl->externalName();
            out << name;
            errors.insert(name);

            first = false;
        }
        error(
            "%s: The following tables are applied in an inconsistent order on different "
            "branches: %s",
            table->srcInfo, out.str());

        // Update tables_reported with the cycle.
        tables_reported.insert(cycle.begin(), cycle.end());
    }

    // Prune the visitor.
    return false;
}

Visitor::profile_t MultipleApply::init_apply(const IR::Node *root) {
    auto result = PassManager::init_apply(root);
    longBranchDisabled = Device::numLongBranchTags() == 0 || options.disable_long_branch;
    return result;
}

MultipleApply::MultipleApply(const BFN_Options &options, std::optional<gress_t> gress,
                             bool dedup_only, bool run_default_next)
    : options(options) {
    addPasses({
        &mutex,
        dedup_only ? nullptr : new MutuallyExclusiveApplies(mutex, mutex_errors),
        new CheckStaticNextTable(*this),
        new DeduplicateTables(*this, gress),
        dedup_only ? nullptr : new CheckTopologicalTables(topological_errors),
        dedup_only || !run_default_next ? nullptr
                                        : new DefaultNext(longBranchDisabled, &default_next_errors),
    });
}
