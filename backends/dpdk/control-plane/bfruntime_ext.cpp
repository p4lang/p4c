/* Copyright 2021 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "bfruntime_ext.h"

namespace P4 {

namespace BFRT {

struct BFRuntimeSchemaGenerator::ActionSelector {
    std::string name;
    std::string get_mem_name;
    P4Id id;
    P4Id get_mem_id;
    P4Id action_profile_id;
    int64_t max_group_size;
    int64_t num_groups;  // aka size of selector
    std::vector<P4Id> tableIds;
    Util::JsonArray *annotations;

    static std::optional<ActionSelector> fromDPDK(
        const p4configv1::P4Info &p4info, const p4configv1::ExternInstance &externInstance) {
        const auto &pre = externInstance.preamble();
        ::dpdk::ActionSelector actionSelector;
        if (!externInstance.info().UnpackTo(&actionSelector)) {
            ::error(ErrorType::ERR_NOT_FOUND,
                    "Extern instance %1% does not pack an ActionSelector object", pre.name());
            return std::nullopt;
        }
        auto selectorId = makeBFRuntimeId(pre.id(), ::dpdk::P4Ids::ACTION_SELECTOR);
        auto selectorGetMemId =
            makeBFRuntimeId(pre.id(), ::dpdk::P4Ids::ACTION_SELECTOR_GET_MEMBER);
        auto tableIds = collectTableIds(p4info, actionSelector.table_ids().begin(),
                                        actionSelector.table_ids().end());
        return ActionSelector{pre.name(),
                              pre.name() + "_get_member",
                              selectorId,
                              selectorGetMemId,
                              actionSelector.action_profile_id(),
                              actionSelector.max_group_size(),
                              actionSelector.num_groups(),
                              tableIds,
                              transformAnnotations(pre)};
    };
};

void BFRuntimeSchemaGenerator::addMatchActionData(const p4configv1::Table &table,
                                                  Util::JsonObject *tableJson,
                                                  Util::JsonArray *dataJson,
                                                  P4Id maxActionParamId) const {
    cstring tableType = tableJson->get("table_type")->to<Util::JsonValue>()->getString();
    if (tableType == "MatchAction_Direct") {
        tableJson->emplace("action_specs", makeActionSpecs(table, &maxActionParamId));
    } else if (tableType == "MatchAction_Indirect") {
        auto *f = makeCommonDataField(BF_RT_DATA_ACTION_MEMBER_ID, "$ACTION_MEMBER_ID",
                                      makeType("uint32"), false /* repeated */);
        addSingleton(dataJson, f, true /* mandatory */, false /* read-only */);
    } else if (tableType == "MatchAction_Indirect_Selector") {
        // action member id and selector group id are mutually-exclusive, so
        // we use a "oneof" here.
        auto *choicesDataJson = new Util::JsonArray();
        choicesDataJson->append(makeCommonDataField(BF_RT_DATA_ACTION_MEMBER_ID,
                                                    "$ACTION_MEMBER_ID", makeType("uint32"),
                                                    false /* repeated */));
        choicesDataJson->append(makeCommonDataField(BF_RT_DATA_SELECTOR_GROUP_ID,
                                                    "$SELECTOR_GROUP_ID", makeType("uint32"),
                                                    false /* repeated */));
        addOneOf(dataJson, choicesDataJson, true /* mandatory */, false /* read-only */);
    } else {
        BUG("Invalid table type '%1%'", tableType);
    }
}

void BFRuntimeSchemaGenerator::addActionSelectorGetMemberCommon(
    Util::JsonArray *tablesJson, const ActionSelector &actionSelector) const {
    auto *tableJson = initTableJson(actionSelector.get_mem_name, actionSelector.get_mem_id,
                                    "SelectorGetMember", 1 /* size */, actionSelector.annotations);

    auto *keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_SELECTOR_GROUP_ID, "$SELECTOR_GROUP_ID", true /* mandatory */,
                "Exact", makeType("uint64"));
    addKeyField(keyJson, BF_RT_DATA_HASH_VALUE, "hash_value", true /* mandatory */, "Exact",
                makeType("uint64"));
    tableJson->emplace("key", keyJson);

    auto *dataJson = new Util::JsonArray();
    {
        auto *f = makeCommonDataField(BF_RT_DATA_ACTION_MEMBER_ID, "$ACTION_MEMBER_ID",
                                      makeType("uint64"), false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    tableJson->emplace("data", dataJson);

    tableJson->emplace("supported_operations", new Util::JsonArray());
    tableJson->emplace("attributes", new Util::JsonArray());
    addToDependsOn(tableJson, actionSelector.id);

    tablesJson->append(tableJson);
}

void BFRuntimeSchemaGenerator::addActionSelectorCommon(Util::JsonArray *tablesJson,
                                                       const ActionSelector &actionSelector) const {
    // TODO(antonin): formalize ID allocation for selector tables
    // repeat same annotations as for action table
    // the maximum number of groups is the table size for the selector table
    auto *tableJson = initTableJson(actionSelector.name, actionSelector.id, "Selector",
                                    actionSelector.num_groups, actionSelector.annotations);

    auto *keyJson = new Util::JsonArray();
    addKeyField(keyJson, BF_RT_DATA_SELECTOR_GROUP_ID, "$SELECTOR_GROUP_ID", true /* mandatory */,
                "Exact", makeType("uint32"));
    tableJson->emplace("key", keyJson);

    auto *dataJson = new Util::JsonArray();
    {
        auto *f = makeCommonDataField(BF_RT_DATA_ACTION_MEMBER_ID, "$ACTION_MEMBER_ID",
                                      makeType("uint32"), true /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    {
        auto *f = makeCommonDataField(BF_RT_DATA_ACTION_MEMBER_STATUS, "$ACTION_MEMBER_STATUS",
                                      makeTypeBool(), true /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    {
        auto *f = makeCommonDataField(BF_RT_DATA_MAX_GROUP_SIZE, "$MAX_GROUP_SIZE",
                                      makeType("uint32", actionSelector.max_group_size),
                                      false /* repeated */);
        addSingleton(dataJson, f, false /* mandatory */, false /* read-only */);
    }
    tableJson->emplace("data", dataJson);

    tableJson->emplace("supported_operations", new Util::JsonArray());
    tableJson->emplace("attributes", new Util::JsonArray());
    addToDependsOn(tableJson, actionSelector.action_profile_id);

    auto oneTableId = actionSelector.tableIds.at(0);
    auto *oneTable = Standard::findTable(p4info, oneTableId);
    CHECK_NULL(oneTable);

    // Add action selector id to match table depends on
    auto oneTableJson = findJsonTable(tablesJson, oneTable->preamble().name());
    addToDependsOn(oneTableJson, actionSelector.id);

    tablesJson->append(tableJson);
}

bool BFRuntimeSchemaGenerator::addActionProfIds(const p4configv1::Table &table,
                                                Util::JsonObject *tableJson) const {
    auto implementationId = table.implementation_id();
    auto actProfId = static_cast<P4Id>(0);
    if (implementationId > 0) {
        auto hasSelector = actProfHasSelector(implementationId);
        if (hasSelector == std::nullopt) {
            ::error(ErrorType::ERR_INVALID, "Invalid implementation id in p4info: %1%",
                    implementationId);
            return false;
        }
        cstring tableType = "";
        if (*hasSelector) {
            tableType = "MatchAction_Indirect_Selector";
            // actSelectorId & actProfId will be set while visiting action profile externs
        } else {
            actProfId = BFRuntimeSchemaGenerator::ActionProf::makeActProfId(implementationId);
            tableType = "MatchAction_Indirect";
        }
        tableJson->erase("table_type");
        tableJson->emplace("table_type", tableType);
    }

    if (actProfId > 0) addToDependsOn(tableJson, actProfId);
    return true;
}

void BFRuntimeSchemaGenerator::addConstTableAttr(Util::JsonArray *) const {
    // Intentionally empty function body to skip attribute addition for const entries
    return;
}

void BFRuntimeSchemaGenerator::addActionProfs(Util::JsonArray *tablesJson) const {
    for (const auto &actionProf : p4info.action_profiles()) {
        auto actionProfInstance = ActionProf::from(p4info, actionProf);
        if (actionProfInstance == std::nullopt) continue;
        addActionProfCommon(tablesJson, *actionProfInstance);
    }
}

bool BFRuntimeSchemaGenerator::addMatchTypePriority(std::optional<cstring> &matchType) const {
    if (*matchType == "Ternary" || *matchType == "Range" || *matchType == "Optional") {
        *matchType = "Ternary";
        return true;
    }
    return false;
}

std::optional<bool> BFRuntimeSchemaGenerator::actProfHasSelector(P4Id actProfId) const {
    if (isOfType(actProfId, p4configv1::P4Ids::ACTION_PROFILE)) {
        auto *actionProf = Standard::findActionProf(p4info, actProfId);
        if (actionProf == nullptr) return std::nullopt;
        return actionProf->with_selector();
    } else if (isOfType(actProfId, ::dpdk::P4Ids::ACTION_SELECTOR)) {
        return true;
    }
    return std::nullopt;
}

const Util::JsonObject *BFRuntimeSchemaGenerator::genSchema() const {
    auto *json = new Util::JsonObject();

    if (isTDI) {
        cstring progName = options.file;
        auto fileName = progName.findlast('/');
        // Handle the case when input file is in the current working directory.
        // fileName would be null in that case, hence progName should remain unchanged.
        if (fileName) progName = fileName;
        auto fileext = progName.find(".");
        progName = progName.replace(fileext, "");
        progName = progName.trim("/\t\n\r");
        json->emplace("program_name", progName);
        json->emplace("build_date", cstring(options.getBuildDate()));
        json->emplace("compile_command", cstring(options.getCompileCommand()));
        json->emplace("compiler_version", cstring(options.compilerVersion));
        json->emplace("schema_version", tdiSchemaVersion);
        json->emplace("target", cstring("DPDK"));
    } else {
        json->emplace("schema_version", bfrtSchemaVersion);
    }

    auto *tablesJson = new Util::JsonArray();
    json->emplace("tables", tablesJson);

    addMatchTables(tablesJson);
    addActionProfs(tablesJson);
    addCounters(tablesJson);
    addMeters(tablesJson);
    addRegisters(tablesJson);

    auto *learnFiltersJson = new Util::JsonArray();
    json->emplace("learn_filters", learnFiltersJson);
    addLearnFilters(learnFiltersJson);

    addDPDKExterns(tablesJson, learnFiltersJson);
    return json;
}

void BFRuntimeSchemaGenerator::addDPDKExterns(Util::JsonArray *tablesJson,
                                              Util::JsonArray *) const {
    for (const auto &externType : p4info.externs()) {
        auto externTypeId = static_cast<::dpdk::P4Ids::Prefix>(externType.extern_type_id());
        if (externTypeId == ::dpdk::P4Ids::ACTION_SELECTOR) {
            for (const auto &externInstance : externType.instances()) {
                auto actionSelector = ActionSelector::fromDPDK(p4info, externInstance);
                if (actionSelector != std::nullopt) {
                    addActionSelectorCommon(tablesJson, *actionSelector);
                    addActionSelectorGetMemberCommon(tablesJson, *actionSelector);
                }
            }
        }
    }
}

}  // namespace BFRT

}  // namespace P4
