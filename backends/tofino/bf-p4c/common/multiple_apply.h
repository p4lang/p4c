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

#ifndef BF_P4C_COMMON_MULTIPLE_APPLY_H_
#define BF_P4C_COMMON_MULTIPLE_APPLY_H_

#include <set>

#include "bf-p4c/ir/thread_visitor.h"
#include "bf-p4c/lib/union_find.hpp"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/table_mutex.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

using namespace P4;

class BFN_Options;

/// The purpose of this set of passes is three-fold:
///
///   * The extract_maupipe code creates a separate Table object for each table call. This set of
///     passes de-duplicates these Table objects so that each table in the program is represented
///     by a single object.
///
///   * It checks the invariant that the conditional control flow that follows each table is the
///     same across all calls of that table. This rules out the following program, in which the
///     conditional behaviour on t2 depends on whether t1 hit or missed.
///
///       if (t1.apply().hit) {
///         switch (t2.apply().action_run) {
///           a1: { t3.apply(); }
///         }
///       } else {
///         switch (t2.apply().action_run) {
///           a1: { t4.apply(); }
///         }
///       }
///
///   * It checks the invariant that the tables have a topological order, as they appear in the
///     program text. This rules out the following program, in which t2 and t3 are applied in the
///     opposite order in two branches of the program.
///
///       switch (t1.apply().action_run) {
///         a1: { t2.apply(); t3.apply(); }
///         a2: { t3.apply(); t2.apply(); }
///       }
///
///     In principle, this invariant can be relaxed to say that the tables must form a DAG in the
///     program dependence graph; i.e., the above program would be admissible if there are no data
///     dependences between t2 and t3. This would require normalizing the order of tables, which we
///     don't currently do. For now, we do the simpler, more restrictive thing, and can do the more
///     relaxed thing in the future.
class MultipleApply : public PassManager {
    // gtest requirements
    std::set<cstring> mutex_errors;
    std::set<cstring> topological_errors;
    std::set<cstring> default_next_errors;

    const BFN_Options &options;

    /// Disable long branches
    bool longBranchDisabled = false;

    /// Pass for computing the mutual-exclusion matrix for tables.
    TablesMutuallyExclusive mutex;

    /// Checks that tables are applied at most once in all paths through the program.
    class MutuallyExclusiveApplies : public MauInspector {
        TablesMutuallyExclusive &mutex;

        /// Any tables failing this mutual-exclusion check will have their names added to this set.
        /// Used by gtest.
        std::set<cstring> &errors;

        /// Maps each source-level P4 table to the Table objects representing the various
        /// invocations of that P4 table.
        ordered_map<const IR::P4Table *, ordered_set<const IR::MAU::Table *>> mutex_apply;

        Visitor::profile_t init_apply(const IR::Node *root) override;
        void postorder(const IR::MAU::Table *) override;

     public:
        explicit MutuallyExclusiveApplies(TablesMutuallyExclusive &m, std::set<cstring> &e)
            : mutex(m), errors(e) {}
    };

    /// Groups together tables that will be de-duplicated.
    UnionFind<const IR::MAU::Table *> duplicate_tables;

    /// Checks the invariant that the conditional control flow that follows each table is the same
    /// across all calls of that table. It also populates \p duplicate_tables
    /// for DeduplicateTables.
    class CheckStaticNextTable : public MauInspector {
        MultipleApply &self;

        /// Gives the canonical Table object for each P4Table.
        ordered_map<const IR::P4Table *, const IR::MAU::Table *> canon_table;

        /// A set of all gateways
        ordered_set<const IR::MAU::Table *> all_gateways;

        /// Marks two Tables as being duplicates and determines whether they are equivalent.
        bool check_equiv(const IR::MAU::Table *table1, const IR::MAU::Table *table2,
                         bool makeUnion = true);

        /// Marks the Tables in two TableSeqs as being duplicates and determines whether they are
        /// equivalent.
        bool check_equiv(const IR::MAU::TableSeq *seq1, const IR::MAU::TableSeq *seq2,
                         bool makeUnion = true);

        /// Determines whether two gateway conditions are equivalent.
        bool equiv_gateway(const IR::Expression *expr1, const IR::Expression *expr2);

        Visitor::profile_t init_apply(const IR::Node *root) override;

        void check_all_gws(const IR::MAU::Table *);
        void postorder(const IR::MAU::Table *) override;

     public:
        explicit CheckStaticNextTable(MultipleApply &s) : self(s) {}
    };

    /// De-duplicates Table and TableSeq objects using \p duplicate_tables.
    class DeduplicateTables : public MauTransform {
        MultipleApply &self;

        /// Provided to the constructor when the visitor is intended to be invoked on a subtree,
        /// rather than the entire IR. If not present, the gress is derived from the visitor's
        /// context. This never changes after construction.
        std::optional<gress_t> gress;

        /// Gives the replacement Table object for each representative table
        /// in \p duplicate_tables.
        std::map<gress_t, std::map<const IR::MAU::Table *, const IR::MAU::Table *>> replacements;

        /// The TableSeq objects we've already produced.
        std::map<gress_t, std::vector<const IR::MAU::TableSeq *>> table_seqs_seen;

        /// @returns the input gress, if provided at construction, or VisitingThread(this).
        gress_t getGress() const { return gress ? *gress : VisitingThread(this); }

        Visitor::profile_t init_apply(const IR::Node *root) override;
        const IR::Node *postorder(IR::MAU::Table *tbl) override;
        const IR::Node *postorder(IR::MAU::TableSeq *seq) override;

     public:
        explicit DeduplicateTables(MultipleApply &s, std::optional<gress_t> gress = std::nullopt)
            : self(s), gress(gress) {}
    };

    /// Checks the invariant that tables have a topological order. This is done by using
    /// FindFlowGraph to build a control-flow graph for the tables in each gress and checking that
    /// there are no cycles.
    ///
    /// This pass assumes that Table objects have been de-duplicated (DeduplicateTables) and that
    /// tables are applied at most once on all paths through the program
    /// (MutuallyExclusiveApplies).
    class CheckTopologicalTables : public MauInspector {
        /// Any tables failing this check will have their names added to this set. Used by gtest.
        std::set<cstring> &errors;

        bool preorder(const IR::MAU::TableSeq *) override;

     public:
        explicit CheckTopologicalTables(std::set<cstring> &errors) : errors(errors) {}
    };

    Visitor::profile_t init_apply(const IR::Node *root) override;

 public:
    bool mutex_error(cstring name) { return mutex_errors.count(name); }

    unsigned num_mutex_errors() { return mutex_errors.size(); }

    bool topological_error(cstring name) { return topological_errors.count(name); }

    unsigned num_topological_errors() { return topological_errors.size(); }

    bool default_next_error(cstring name) {
        return default_next_errors.find(name) != default_next_errors.end();
    }

    unsigned num_default_next_errors() { return default_next_errors.size(); }

    // For gtests, allow doing just the passes for de-duplicating tables and allow specifying which
    // gress is being visited.
    MultipleApply(const BFN_Options &options, std::optional<gress_t> gress = std::nullopt,
                  bool dedup_only = false, bool run_default_next = true);
};

#endif /* BF_P4C_COMMON_MULTIPLE_APPLY_H_ */
