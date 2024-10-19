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
