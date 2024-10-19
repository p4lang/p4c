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

#include "add_always_run.h"

#include "bf-p4c/common/empty_tableseq.h"
#include "bf-p4c/ir/table_tree.h"

int AddAlwaysRun::compare(const IR::MAU::Table *t1, const IR::MAU::Table *t2) const {
    return compare(t1, t2 ? std::optional<UniqueId>(t2->get_uid()) : std::nullopt);
}

int AddAlwaysRun::compare(const IR::MAU::Table *t1, std::optional<UniqueId> t2) const {
    if (t1 == nullptr) return t2 ? 1 : 0;
    if (!t2) return -1;

    BUG_CHECK(globalOrderings.count(t1->gress), "Global table ordering not available for %1%",
              t1->gress);

    auto globalOrdering = globalOrderings.at(t1->gress);

    auto t1Id = t1->get_uid();
    auto t2Id = *t2;
    BUG_CHECK(globalOrdering.count(t1Id), "Global ordering not available for table %1%", t1);
    BUG_CHECK(globalOrdering.count(t2Id), "Global ordering not available for table %1% in %2%",
              t2Id, t1->gress);

    return globalOrdering.at(t1Id) - globalOrdering.at(t2Id);
}

Visitor::profile_t AddAlwaysRun::PrepareToAdd::init_apply(const IR::Node *root) {
    auto result = MauInspector::init_apply(root);

    // Start with a clean slate.
    minSubsequentTables.clear();
    self.globalOrderings.clear();
    self.subsequentTables.clear();

    for (auto &gress_constraintMap : self.allTablesToAdd) {
        auto &gress = gress_constraintMap.first;
        auto &constraintMap = gress_constraintMap.second;

        // Get the flow graph for the current gress and populate an index of tables in the flow
        // graph by their unique ID.
        auto &flowGraph = self.flowGraphs[gress];
        std::map<UniqueId, const IR::MAU::Table *> tablesByUniqueId;
        for (auto *table : flowGraph.get_tables()) {
            tablesByUniqueId[table->get_uid()] = table;
        }

        // Add the always-run tables to the index and to the flow graph.
        for (auto &table : Keys(constraintMap)) {
            tablesByUniqueId[table->pp_unique_id()] = table;
            flowGraph.add_vertex(table);
        }

        // Add dependency edges between the tables to the flow graph.
        for (auto &table_constraints : constraintMap) {
            auto &table = table_constraints.first;

            if (LOGGING(2)) {
                LOG2("\tAlways Run Table " << table->name);
                for (auto action : Values(table->actions)) {
                    LOG3("\t  action " << action->name);
                    for (auto instr : action->action) LOG3("\t    " << instr);
                }
            }

            auto &constraints = table_constraints.second;

            auto &tableIdsBefore = constraints.first;
            auto &tableIdsAfter = constraints.second;

            for (auto &beforeId : tableIdsBefore) {
                auto *beforeTable = tablesByUniqueId.at(beforeId);
                LOG2("\t    Before Table " << beforeTable->name);
                flowGraph.add_edge(beforeTable, table, "always_run"_cs);
            }

            for (auto &afterId : tableIdsAfter) {
                auto *afterTable = tablesByUniqueId.at(afterId);
                LOG2("\t    After Table " << afterTable->name);
                flowGraph.add_edge(table, afterTable, "always_run"_cs);
            }
        }

        // Do a topological sort of the flow graph to populate self.globalOrderings.
        int pos = 0;
        for (auto *table : flowGraph.topological_sort()) {
            if (table) self.globalOrderings[gress][table->get_uid()] = pos++;
        }
    }

    return result;
}

bool AddAlwaysRun::PrepareToAdd::preorder(const IR::MAU::TableSeq *tableSeq) {
    // Override the behaviour for visiting table sequences so that we accurately track
    // subsequentTable.

    // Bypass visiting this table sequence if no tables will be added to the current gress.
    if (tableSeq->tables.size() && !self.allTablesToAdd.count(tableSeq->tables.at(0)->gress)) {
        return false;
    }

    // Ensure we re-visit this table sequence if we encounter it again elsewhere in the IR.
    visitAgain();

    const auto *savedSubsequentTable = subsequentTable;

    for (unsigned i = 0; i < tableSeq->tables.size(); ++i) {
        // subsequentTable is the next table in the sequence.
        // If we're at the last table, then it's the one that we saved above.
        subsequentTable =
            i < tableSeq->tables.size() - 1 ? tableSeq->tables.at(i + 1) : savedSubsequentTable;

        // Visit the current table.
        visit(tableSeq->tables.at(i));
    }

    return false;
}

bool AddAlwaysRun::PrepareToAdd::preorder(const IR::MAU::Table *table) {
    // Ensure we re-visit this table if we encounter it again elsewhere in the IR.
    visitAgain();

    auto *curMin = ::get(minSubsequentTables, table);
    if (self.compare(subsequentTable, curMin) < 0) curMin = subsequentTable;
    minSubsequentTables[table] = curMin;

    return true;
}

void AddAlwaysRun::PrepareToAdd::end_apply(const IR::Node *root) {
    Visitor::end_apply(root);

    // Convert minSubsequentTables into self.subsequentTables.
    for (auto &entry : minSubsequentTables) {
        auto *table = entry.first;
        auto *subsequentTable = entry.second;

        self.subsequentTables[table->get_uid()] =
            subsequentTable ? std::optional<UniqueId>(subsequentTable->get_uid()) : std::nullopt;
    }
}

const IR::BFN::Pipe *AddAlwaysRun::AddTables::preorder(IR::BFN::Pipe *pipe) {
    // Override the behaviour for visiting pipes so that we accurately update tablesToAdd for each
    // gress.
    prune();

    std::initializer_list<std::pair<const IR::MAU::TableSeq *&, gress_t>> threads = {
        {pipe->thread[0].mau, INGRESS},
        {pipe->thread[1].mau, EGRESS},
        {pipe->ghost_thread.ghost_mau, GHOST},
    };
    for (auto &mau_gress : threads) {
        auto *&mau = mau_gress.first;
        auto gress = mau_gress.second;

        // Skip this gress if nothing to add.
        if (!self.globalOrderings.count(gress)) continue;

        // Build a map of tables to add, indexed by unique ID.
        std::map<UniqueId, const IR::MAU::Table *> tableIdx;
        for (auto *table : Keys(self.allTablesToAdd.at(gress))) {
            tableIdx[table->get_uid()] = table;
        }

        // Populate tablesToAdd.
        for (auto &tableId : Keys(self.globalOrderings.at(gress))) {
            if (tableIdx.count(tableId)) tablesToAdd.push_back(tableIdx.at(tableId));
        }

        // Visit the gress.
        LOG6("AddAlwaysRun::AddTables before" << Log::endl << TableTree(toString(gress), mau));
        visit(mau);
        LOG6("AddAlwaysRun::AddTables after" << Log::endl << TableTree(toString(gress), mau));
    }

    return pipe;
}

const IR::Node *AddAlwaysRun::AddTables::preorder(IR::MAU::TableSeq *tableSeq) {
    // Override the behaviour for visiting table sequences so that we accurately track
    // subsequentTable. This is also where we insert the always-run tables into the IR.
    prune();

    // Ensure we re-visit this table sequence if we encounter it again elsewhere in the IR.
    visitAgain();

    const auto savedSubsequentTable = subsequentTable;

    // This will hold the result that we will use to overwrite the table sequence being visited.
    IR::Vector<IR::MAU::Table> result;

    for (unsigned i = 0; i < tableSeq->tables.size(); ++i) {
        auto *curTable = tableSeq->tables[i];

        // subsequentTable is the current table's minimum subsequent table.
        // If we're at the last table, then it's the one that we saved above.
        subsequentTable = i < tableSeq->tables.size() - 1
                              ? self.subsequentTables.at(curTable->get_uid())
                              : savedSubsequentTable;

        // Add to the result any tables that need to come before the current table.
        while (!tablesToAdd.empty() && self.compare(tablesToAdd.front(), curTable) < 0) {
            result.push_back(tablesToAdd.front());
            tablesToAdd.pop_front();
        }

        // Visit the current table. This mirrors the functionality in IR::Vector::visit_children.
        auto *rewritten = apply_visitor(curTable);
        if (!rewritten && curTable) {
            continue;
        } else if (rewritten == curTable) {
            result.push_back(curTable);
        } else if (auto l = dynamic_cast<const IR::Vector<IR::MAU::Table> *>(rewritten)) {
            result.append(*l);
        } else if (auto v = dynamic_cast<const IR::VectorBase *>(rewritten)) {
            if (v->empty()) continue;

            for (auto elt : *v) {
                if (auto e = dynamic_cast<const IR::MAU::Table *>(elt)) {
                    result.push_back(e);
                } else {
                    BUG("visitor returned invalid type %s for Vector<IR::MAU::Table>",
                        elt->node_type_name());
                }
            }
        } else if (auto e = dynamic_cast<const IR::MAU::Table *>(rewritten)) {
            result.push_back(e);
        } else {
            BUG("visitor returned invalid type %s for Vector<IR::MAU::Table>",
                rewritten->node_type_name());
        }
    }

    // Append to the result any tables that need to come before the subsequentTable.
    while (!tablesToAdd.empty() && self.compare(tablesToAdd.front(), subsequentTable) < 0) {
        result.push_back(tablesToAdd.front());
        tablesToAdd.pop_front();
    }

    tableSeq->tables = result;
    return tableSeq;
}

AddAlwaysRun::AddTables *AddAlwaysRun::AddTables::clone() const { return new AddTables(*this); }

AddAlwaysRun::AddTables &AddAlwaysRun::AddTables::flow_clone() { return *clone(); }

void AddAlwaysRun::AddTables::flow_merge(Visitor &v_) {
    // Check that the other visitor agrees with this one on the tables that need to be added.
    AddTables &v = dynamic_cast<AddTables &>(v_);
    BUG_CHECK(tablesToAdd == v.tablesToAdd, "Inconsistent tables added on merging program paths");
}

void AddAlwaysRun::AddTables::flow_copy(::ControlFlowVisitor &v_) {
    // Check that the other visitor agrees with this one on the tables that need to be added.
    AddTables &v = dynamic_cast<AddTables &>(v_);
    BUG_CHECK(tablesToAdd == v.tablesToAdd, "Inconsistent tables added on merging program paths");
}

AddAlwaysRun::AddAlwaysRun(const ordered_map<gress_t, ConstraintMap> &tablesToAdd)
    : allTablesToAdd(tablesToAdd) {
    addPasses({
        // Rewrite the IR to add empty table sequences for fall-through branches.
        new AddEmptyTableSeqs(),

        // Obtain a flow graph for each gress.
        new FindFlowGraphs(flowGraphs),

        // Insert the requested tables into the flow graph according to the input constraints,
        // then topologically sort the tables in the resulting graph to obtain a global ordering
        // for all tables. Using this ordering, determine the earliest "subsequent" table for each
        // table in the program.
        new PrepareToAdd(*this),

        // Insert the tables into the IR so that all execution paths are consistent with the global
        // ordering.
        new AddTables(*this),
    });
}
