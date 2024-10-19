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

#ifndef BF_P4C_MAU_ADD_ALWAYS_RUN_H_
#define BF_P4C_MAU_ADD_ALWAYS_RUN_H_

#include <optional>

#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/mau/table_flow_graph.h"
#include "bf-p4c/phv/phv_fields.h"

using namespace P4;

/// Adds a set of Tables to the IR so that they are executed on all paths through the program. As
/// input, each inserted table comes with a set of constraints, specifying which pipeline units
/// (i.e., parser, table, or deparser) the inserted table must come before or after.
class AddAlwaysRun : public PassManager {
    friend class AddAlwaysRunTest;

 private:
    /// The tables to be added for each gress, mapped to the tables' constraints.
    const ordered_map<gress_t, ConstraintMap> &allTablesToAdd;

    /// Holds the control-flow graph for each gress in which a table is to be added. Cleared and
    /// recomputed every time this pass manager is run.
    ordered_map<gress_t, FlowGraph> flowGraphs;

    /// A global ordering of the tables in each gress, including the tables being added. When
    /// tables are inserted into the IR, it is done so that all execution paths are consistent with
    /// this ordering. The ordering is represented as a map from the unique ID for each table to
    /// the table's position in the ordering. Only contains gresses that have tables being added.
    /// Cleared and recomputed every time this pass manager is run.
    std::map<gress_t, ordered_map<const UniqueId, unsigned>> globalOrderings;

    /**
     * Maps the unique ID for each table to the unique ID of its earliest "subsequent" table, if
     * any. Here, a subsequent table for a table T is one that executes immediately after T and any
     * control flow dependent on T.
     *
     * For example, in the following IR fragment, t1's, t3's and t6's subsequent table is t7. For
     * t2, t4, and t5, the subsequent table is t3. Table t7 has no subsequent table.
     *
     *          [ t1  t7 ]
     *           /  \
     *    [t2  t3]  [t6]
     *    /  \
     * [t4]  [t5]
     *
     * Tables that are applied in multiple branches in the program may have multiple subsequent
     * tables. This data structure stores the earliest one, according to globalOrderings. For an
     * example of why this needs to be done, see the gtest AddAlwaysRunTest.MultipleApply1.
     *
     * This map omits tables in gresses in which no tables will be added.
     */
    std::map<const UniqueId, std::optional<UniqueId>> subsequentTables;

    /// Comparison for Tables according to globalOrderings. The result is less than 0 if t1 comes
    /// before t2, equal to 0 if t1 is the same as t2, and greater than 0 otherwise. Think of this
    /// as returning something like "t1 - t2". Tables that are nullptr are considered to come after
    /// all other tables.
    int compare(const IR::MAU::Table *t1, const IR::MAU::Table *t2) const;
    int compare(const IR::MAU::Table *t1, std::optional<UniqueId> t2) const;

    /// Prepares for insertion by doing some precomputation to populate globalOrderings and
    /// subsequentTables.
    class PrepareToAdd : public MauInspector {
        AddAlwaysRun &self;

        /// The subsequent table for the current program point being visited.
        const IR::MAU::Table *subsequentTable = nullptr;

        /// Maps each table to its earliest subsequent table.
        std::map<const IR::MAU::Table *, const IR::MAU::Table *> minSubsequentTables;

        profile_t init_apply(const IR::Node *root) override;
        bool preorder(const IR::MAU::TableSeq *) override;
        bool preorder(const IR::MAU::Table *) override;
        void end_apply(const IR::Node *root) override;

     public:
        explicit PrepareToAdd(AddAlwaysRun &self) : self(self) {}
    };

    /// Performs the actual insertion of tables into the IR.
    class AddTables : public MauTransform, ControlFlowVisitor {
        AddAlwaysRun &self;

        /// The list of remaining always-run tables to be added to the IR for the current gress
        /// being visited. This is sorted according to the order given by globalOrderings. Tables
        /// are popped off the front of this list as they are added to the IR.
        //
        // Invariant: this is empty at the start and end of each visit to each gress.
        std::list<const IR::MAU::Table *> tablesToAdd;

        /// A lower bound for the subsequent table for the current program point being visited.
        std::optional<UniqueId> subsequentTable;

        const IR::BFN::Pipe *preorder(IR::BFN::Pipe *) override;
        const IR::Node *preorder(IR::MAU::TableSeq *) override;

        AddTables *clone() const override;
        AddTables &flow_clone() override;
        void flow_merge(Visitor &v) override;
        void flow_copy(::ControlFlowVisitor &v) override;

        AddTables(const AddTables &) = default;
        AddTables(AddTables &&) = default;

     public:
        explicit AddTables(AddAlwaysRun &self) : self(self) {}
    };

 public:
    explicit AddAlwaysRun(const ordered_map<gress_t, ConstraintMap> &tablesToAdd);
};

#endif /* BF_P4C_MAU_ADD_ALWAYS_RUN_H_ */
