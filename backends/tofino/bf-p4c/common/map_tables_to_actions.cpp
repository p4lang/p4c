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

#include "bf-p4c/common/map_tables_to_actions.h"

Visitor::profile_t MapTablesToActions::init_apply(const IR::Node *root) {
    tableToActionsMap.clear();
    defaultActions.clear();
    actionMap.clear();
    return Inspector::init_apply(root);
}

const PHV::ActionSet &MapTablesToActions::getActionsForTable(const IR::MAU::Table *t) const {
    BUG_CHECK(t, "Null table encountered");
    static const PHV::ActionSet emptySet;
    if (!tableToActionsMap.count(t)) return emptySet;
    return tableToActionsMap.at(t);
}

const PHV::ActionSet &MapTablesToActions::getDefaultActionsForTable(const IR::MAU::Table *t) const {
    BUG_CHECK(t, "Null table encountered");
    static const PHV::ActionSet emptySet;
    if (!defaultActions.count(t)) return emptySet;
    return defaultActions.at(t);
}

std::optional<const IR::MAU::Table *> MapTablesToActions::getTableForAction(
    const IR::MAU::Action *act) const {
    BUG_CHECK(act, "Null action encountered.");
    if (actionMap.count(act)) {
        return actionMap.at(act);
    } else {
        // This is to workaround a problem with the getInitPoints() from AllocSlice that carry
        // old IR::MAU::Action pointer. Using the clone_id instead is less efficient but resilient
        // over IR transformation.
        for (auto kv : actionMap) {
            if (kv.first->clone_id == act->clone_id) return kv.second;
        }
    }
    return std::nullopt;
}

bool MapTablesToActions::preorder(const IR::MAU::Table *t) {
    for (auto kv : t->actions) {
        const auto *action = kv.second;
        tableToActionsMap[t].insert(action);
        actionMap[action] = t;
        LOG6("\tAdd action " << action->name << " in table " << t->name);
        if (action->miss_only() || action->init_default) defaultActions[t].insert(kv.second);
    }
    return true;
}

void MapTablesToActions::printTableActionsMap(const MapTablesToActions::TableActionsMap &tblActMap,
                                              cstring logMessage) const {
    LOG5("\t  " << logMessage);
    for (auto kv : tblActMap) {
        std::stringstream ss;
        ss << "\t\t" << kv.first->name << "\t:\t";
        for (const auto *act : kv.second) ss << act->name << " ";
        LOG5(ss.str());
    }
}

void MapTablesToActions::end_apply() {
    if (LOGGING(5)) {
        printTableActionsMap(tableToActionsMap, "Printing tables to actions map"_cs);
        printTableActionsMap(defaultActions, "Printing tables to default actions map"_cs);
        LOG5("Printing action to tables map");
        for (auto kv : actionMap) LOG5("\t" << kv.first->name << "\t:\t" << kv.second->name);
    }
}
