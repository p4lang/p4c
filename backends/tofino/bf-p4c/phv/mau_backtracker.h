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

#ifndef BF_P4C_PHV_MAU_BACKTRACKER_H_
#define BF_P4C_PHV_MAU_BACKTRACKER_H_

#include "ir/ir.h"
#include "lib/ordered_map.h"
#include "bf-p4c/mau/table_summary.h"

/** When backtracking, this class contains members that save the generated table placement (without
  * container conflicts) for use by the second round of PHV allocation
  */
class MauBacktracker : public Backtrack {
 private:
    State::state_t &state;
    static constexpr unsigned NUM_LOGICAL_TABLES_PER_STAGE = 16;
    /// To keep track of the number of times this pass has been invoked
    int numInvoked = 0;

    /// Store a map of table names to stage, used as reference by the second round of PHV allocation
    /// (after a backtrack exception has been thrown by TableSummary)
    ordered_map<cstring, ordered_set<int>> tables;
    /// Map of table names to stage from a previous round without container conflicts.
    ordered_map<cstring, ordered_set<int>> prevRoundTables;

    /// Table Order for 2nd TP round in alt phv alloc
    ordered_map<int, ordered_set<ordered_set<cstring> > > table_order;

    /// Store a map of internal table names to stage, used as reference by the second round of
    /// PHV allocation (after a backtrack exception has been thrown by TableSummary). Only used
    /// on stage() function with internal == true.
    /// Also used in 2nd round of Alt PHV Alloc Table Placement
    ordered_map<cstring, ordered_set<int>> internalTables;

    /// Store a map of internal table names to merged condition tables with the
    /// gateway tag, used as reference by the second round of Alt PHV Alloc
    /// Table Placement
    ordered_map<cstring, std::pair<cstring, cstring>> mergedGateways;

    // true if a valid table placement is stored in tables. NOTE: we need this variable instead
    // of just using !tables.empty() because for program with no table, an empty alloc is valid.
    bool has_table_placement = false;

    /// Store the number of stages required by table allocation
    int maxStage = -1;

    /// True if metadata initialization must be disabled.
    bool metaInitDisable = false;

    /// True if PHV allocation must be redone while ignoring pack conflicts.
    bool ignorePackConflicts = false;

    /// True if PHV allocation and table placement both fit in round 1.
    bool firstRoundFit = false;

    /// TABLE PLACEMENT FIRST
    /// Post Table Placement Round, table summary pass runs and populates all
    /// resource allocation on a per stage basis. PHV Allocation Round can use this
    /// information to assist in identifying PHV constraints associated with
    /// input_xbar, live ranges etc.
    TableSummary *table_summary;

    const IR::Node *apply_visitor(const IR::Node *root, const char *) override;

    /// This is the function that catches the backtracking exception from TableSummary. This should
    /// return true.
    bool backtrack(trigger &) override;

    ordered_set<int> inSameStage(
            const IR::MAU::Table* t1,
            const IR::MAU::Table* t2,
            const ordered_map<cstring, ordered_set<int>>& tableMap) const;

 public:
    /// @returns the stage number(s) if tables @p t1 and @p t2 are placed in the same stage
    /// @returns an empty set if tables are not in the same stage
    ordered_set<int> inSameStage(const IR::MAU::Table* t1, const IR::MAU::Table* t2) const;

    /// Prints the table allocation received by MauBacktracker by means of the backtrack trigger
    std::string printTableAlloc() const;

    /// @returns true if the MauBacktracker class has any information about table placement (found
    /// in the tables map
    bool hasTablePlacement() const;

    /// @returns the stages in which table @p t was placed. Use internalTables mapping when internal
    /// flag is set.
    ordered_set<int> stage(const IR::MAU::Table* t, bool internal = false) const;

    /// @returns metaInitDisable.
    bool disableMetadataInitialization() const { return metaInitDisable; }

    /// @returns firstRoundFit.
    bool didFirstRoundFit() const { return firstRoundFit; }

    bool happensBefore(const IR::MAU::Table* t1, const IR::MAU::Table* t2) const;

    /// @returns the number of stages in the table allocation
    int numStages() const;

    /// @returns the Table Summary pointer
    const TableSummary* get_table_summary() const { return table_summary; };

    /// @returns tables structure
    const ordered_map<cstring, ordered_set<int>>&
        getInternalTablesInfo() const { return internalTables; }

    /// @returns merged gateways structure
    const ordered_map<cstring, std::pair<cstring, cstring>>&
        getMergedGatewaysInfo() const { return mergedGateways; }

    /// Clear the MauBacktracker state.
    /// Only use when backtracking to a point where everything should be reset.
    /// We pass state back through this object in many cases -- don't clear in those cases.
    void clear();

    /// Constructor takes mutually exclusive to be able to clear it before every
    /// PHV allocation pass
    explicit MauBacktracker(State::state_t &state = *(new State::state_t),
        TableSummary *table_summary = nullptr)
        : state(state), table_summary(table_summary) {}
};

#endif /* BF_P4C_PHV_MAU_BACKTRACKER_H_ */
