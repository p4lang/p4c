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

#include "bf-p4c/phv/allocate_phv.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "lib/log.h"

bool MauBacktracker::backtrack(trigger &trig) {
    if (trig.is<PHVTrigger::failure>()) {
        auto t = dynamic_cast<PHVTrigger::failure *>(&trig);
        LOG1("Backtracking caught at MauBacktracker");
        // The OR operation ensures that live range analysis and metadata initialization, once
        // disabled, is not enabled for the rest of the compilation process.
        metaInitDisable |= t->metaInitDisable;
        ignorePackConflicts |= t->ignorePackConflicts;
        firstRoundFit |= t->firstRoundFit;
        has_table_placement = true;
        // If we are directed to ignore pack conflicts, then do not note down the previous table
        // placement.
        LOG4("Already existing tables size: " << tables.size());
        if (!ignorePackConflicts) {
            if (tables.size() > 0) {
                // There exists already a table placement from a previous round without container
                // conflicts.
                for (auto entry : tables)
                    prevRoundTables[entry.first] = entry.second;
            }
            tables.clear();
            for (auto entry : t->tableAlloc) {
                tables[entry.first] = entry.second;
                for (int logical_id : entry.second) {
                    int st = logical_id / NUM_LOGICAL_TABLES_PER_STAGE;
                    maxStage = (maxStage < st) ? st : maxStage; }
            }
            internalTables.clear();
            for (auto entry : t->internalTableAlloc)
                internalTables[entry.first] = entry.second;
            mergedGateways = t->mergedGateways;
            LOG4("Inserted tables size: " << tables.size());
        } else {
            LOG4("IgnorePackConflicts is true, no table placement inserted.");
        }
        if (t->stopTableReplayFitting) {
            // stopTableReplayFitting has thrown, jump to ALT_FINALIZE_TABLE and prepare for
            // normal phv allocation and table allocation.
            BUG_CHECK(state == State::ALT_FINALIZE_TABLE_SAME_ORDER_TABLE_FIXED,
                "wrong compilation state");
            state = State::ALT_FINALIZE_TABLE;
            table_summary->clear_table_alloc_info();
        }
        return true;
    }
    return false;
}

const IR::Node *MauBacktracker::apply_visitor(const IR::Node* root, const char *) {
    LOG1("MauBacktracker called " << numInvoked << " time(s)");
    LOG1("  Is metadata initialization disabled? " << (metaInitDisable ? "YES" : "NO"));
    LOG1("  Should pack conflicts be ignored? " << (ignorePackConflicts ? "YES" : "NO"));
    ++numInvoked;
    LOG4("    Size of this round tables: " << tables.size());
    LOG4("    Size of previous round tables: " << prevRoundTables.size());
    if (firstRoundFit) {
        LOG4("  Clearing all logging");
        tables.clear();
        prevRoundTables.clear(); }
    if (numInvoked != 1)
        LOG4(printTableAlloc());
    return root;
}

ordered_set<int>
MauBacktracker::inSameStage(const IR::MAU::Table* t1, const IR::MAU::Table* t2) const {
    BUG_CHECK(t1, "Null table!");
    BUG_CHECK(t2, "Null table!");
    ordered_set<int> rs;
    auto thisRound = inSameStage(t1, t2, tables);
    rs.insert(thisRound.begin(), thisRound.end());
    auto prevRound = inSameStage(t1, t2, prevRoundTables);
    rs.insert(prevRound.begin(), prevRound.end());
    return rs;
}

ordered_set<int>
MauBacktracker::inSameStage(
        const IR::MAU::Table* t1,
        const IR::MAU::Table* t2,
        const ordered_map<cstring, ordered_set<int>>& tableMap) const {
    ordered_set<int> rs;
    if (tableMap.size() == 0) return rs;
    // Some tables in the list of tableActions maintained by PackConflicts may not have an
    // allocation (dead code eliminated away). The following if condition handles that case.
    if (!tableMap.count(TableSummary::getTableName(t1)) ||
            !tableMap.count(TableSummary::getTableName(t2)))
        return rs;
    const ordered_set<int>& t1Stages = tableMap.at(TableSummary::getTableName(t1));
    const ordered_set<int>& t2Stages = tableMap.at(TableSummary::getTableName(t2));
    BUG_CHECK(t1Stages.size() > 0, "No allocation for table %1%", t1->name);
    BUG_CHECK(t2Stages.size() > 0, "No allocation for table %2%", t2->name);
    for (int a : t1Stages) {
        for (int b : t2Stages) {
            int stage_a = a / NUM_LOGICAL_TABLES_PER_STAGE;
            int stage_b = b / NUM_LOGICAL_TABLES_PER_STAGE;
            if (stage_a == stage_b) rs.insert(stage_a); } }
    return rs;
}

std::string MauBacktracker::printTableAlloc() const {
    std::stringstream talloc;
    // if (!LOGGING(3)) return;
    talloc << "Printing Table Placement Before PHV Allocation Pass" << std::endl;
    talloc << "Stage | Logical Id | Table Name" << std::endl;
    for (auto tbl : tables) {
        for (int logical_id : tbl.second) {
            int st = logical_id / NUM_LOGICAL_TABLES_PER_STAGE;
            int id = logical_id % NUM_LOGICAL_TABLES_PER_STAGE;
            talloc << boost::format("%5d")  % st << " | "
                   << boost::format("%10d") % id << " | "
                   << tbl.first << std::endl;
        }
    }
    return talloc.str();
}

bool MauBacktracker::hasTablePlacement() const {
    return has_table_placement;
}

ordered_set<int> MauBacktracker::stage(const IR::MAU::Table* t, bool internal) const {
    ordered_set<int> rs;
    if (internal) {
        if (internalTables.size() == 0) return rs;
        auto tbl_name = TableSummary::getTableIName(t);
        if (!internalTables.count(tbl_name)) return rs;
        for (auto logical_id : internalTables.at(tbl_name))
            rs.insert(logical_id / NUM_LOGICAL_TABLES_PER_STAGE);
    } else {
        if (tables.size() == 0) return rs;
        cstring tableName = TableSummary::getTableName(t);
        if (!tables.count(tableName)) return rs;
        for (auto logical_id : tables.at(tableName))
            rs.insert(logical_id / NUM_LOGICAL_TABLES_PER_STAGE);
    }
    return rs;
}

bool MauBacktracker::happensBefore(
        const IR::MAU::Table* t1,
        const IR::MAU::Table* t2) const {
    if (tables.size() == 0) return true;
    if (maxStage == -1 || maxStage >= Device::numStages()) return true;
    cstring tableName1 = TableSummary::getTableName(t1);
    cstring tableName2 = TableSummary::getTableName(t2);
    if (!tables.count(tableName1) || !tables.count(tableName2)) return true;
    for (auto logical_id_1 : tables.at(tableName1)) {
        for (auto logical_id_2 : tables.at(tableName2)) {
            if (logical_id_1 >= logical_id_2) return false;
            int stage1 = logical_id_1 / NUM_LOGICAL_TABLES_PER_STAGE;
            int stage2 = logical_id_2 / NUM_LOGICAL_TABLES_PER_STAGE;
            if (stage1 >= stage2) return false;
        }
    }
    return true;
}

int MauBacktracker::numStages() const {
    return maxStage;
}

void MauBacktracker::clear() {
    tables.clear();
    prevRoundTables.clear();
    maxStage = -1;
    metaInitDisable = false;
    ignorePackConflicts = false;
    firstRoundFit = false;
}
