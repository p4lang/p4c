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
