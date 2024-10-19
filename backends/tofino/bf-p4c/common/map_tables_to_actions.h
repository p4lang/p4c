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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_MAP_TABLES_TO_ACTIONS_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_MAP_TABLES_TO_ACTIONS_H_

#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

using namespace P4;

/** Create maps of tables to associated actions and associated default actions.
 */
class MapTablesToActions : public Inspector {
 public:
    using TableActionsMap = ordered_map<const IR::MAU::Table *, PHV::ActionSet>;
    using ActionTableMap = ordered_map<const IR::MAU::Action *, const IR::MAU::Table *>;

 private:
    /// tableToActionsMap[t] = Set of actions that can be invoked by table t.
    TableActionsMap tableToActionsMap;

    /// defaultActions[t] = Set of default actions for table t.
    TableActionsMap defaultActions;

    /// actionMap[act] = t, where t is the table from which act is invoked.
    ActionTableMap actionMap;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::Table *t) override;
    void end_apply() override;

    /// Pretty-print maps of type TableActionsMap.
    void printTableActionsMap(const TableActionsMap &tblActMap, cstring logMessage) const;

 public:
    /// @returns the set of actions that can be invoked for a table @p t.
    const PHV::ActionSet &getActionsForTable(const IR::MAU::Table *t) const;

    /// @returns the set of possible default actions for table @p t.
    const PHV::ActionSet &getDefaultActionsForTable(const IR::MAU::Table *t) const;

    /// @return the table from which @p act is invoked.
    std::optional<const IR::MAU::Table *> getTableForAction(const IR::MAU::Action *act) const;
};

#endif /*  BACKENDS_TOFINO_BF_P4C_COMMON_MAP_TABLES_TO_ACTIONS_H_  */
