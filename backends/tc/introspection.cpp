/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#include "introspection.h"

/// This file defines functions for the pass to generate the introspection file

namespace TC {

void IntrospectionGenerator::postorder(const IR::P4Table *t) {
    p4tables.emplace(t->name.originalName, t);
}

cstring IntrospectionGenerator::externalName(const IR::IDeclaration *declaration) {
    cstring name = declaration->externalName();
    if (name.startsWith(".")) name = name.substr(1);
    auto Name = name.replace('.', '/');
    return Name;
}

void IntrospectionGenerator::collectTableInfo() {
    for (auto table : tcPipeline->tableDefs) {
        auto tableInfo = new struct TableAttributes();
        tableInfo->id = table->tableID;
        tableInfo->name = table->controlName + "/" + table->tableName;
        tableInfo->tentries = table->tableEntriesCount;
        tableInfo->numMask = table->numMask;
        if (table->keySize != 0) {
            tableInfo->keysize = table->keySize;
        }
        const IR::P4Table *p4table = nullptr;
        p4table = p4tables[table->tableName];
        if (p4table == nullptr) continue;
        // Key field information collection
        auto key = p4table->getKey();
        if (key != nullptr && key->keyElements.size()) collectKeyInfo(key, tableInfo);
        // Action information collection
        auto actionlist = p4table->getActionList();
        if (actionlist != nullptr) collectActionInfo(actionlist, tableInfo, p4table, table);
        tablesInfo.push_back(tableInfo);
    }
}

void IntrospectionGenerator::collectKeyInfo(const IR::Key *key, struct TableAttributes *tableInfo) {
    unsigned int i = 1;
    for (auto k : key->keyElements) {
        auto keyField = new struct KeyFieldAttributes();
        keyField->id = i++;
        auto keyExp = k->expression;
        keyField->name = keyExp->toString();
        keyField->matchType = k->matchType->toString();
        auto keyExpType = typeMap->getType(keyExp);
        auto widthBits = keyExpType->width_bits();
        keyField->type = "bit" + Util::toString(widthBits);
        auto keyAnno = k->getAnnotations()->annotations;
        for (auto anno : keyAnno) {
            if (anno->name == ParseTCAnnotations::tcType) {
                auto expr = anno->expr[0];
                if (auto typeLiteral = expr->to<IR::StringLiteral>()) {
                    if (auto tcVal = checkValidTcType(typeLiteral)) {
                        auto val = std::move(*tcVal);
                        keyField->type = val;
                    } else {
                        ::error(ErrorType::ERR_INVALID,
                                "tc_type annotation cannot have '%1%' as value", expr);
                    }
                } else {
                    ::error(ErrorType::ERR_INVALID, "tc_type annotation cannot have '%1%' as value",
                            expr);
                }
            }
            if (anno->name == IR::Annotation::nameAnnotation) {
                auto expr = anno->expr[0];
                if (auto name = expr->to<IR::StringLiteral>()) {
                    keyField->name = name->value;
                }
            }
        }
        keyField->bitwidth = widthBits;
        tableInfo->keyFields.push_back(keyField);
    }
}

void IntrospectionGenerator::collectActionInfo(const IR::ActionList *actionlist,
                                               struct TableAttributes *tableInfo,
                                               const IR::P4Table *p4table,
                                               const IR::TCTable *table) {
    for (auto action : actionlist->actionList) {
        for (auto actionDef : tcPipeline->actionDefs) {
            auto adecl = refMap->getDeclaration(action->getPath(), true);
            auto actionName = externalName(adecl);
            if (actionName != actionDef->actionName) continue;
            auto actionInfo = new struct ActionAttributes();
            actionInfo->id = actionDef->actId;
            actionInfo->name = actionDef->actionName;
            auto annoList = action->getAnnotations()->annotations;
            bool isTableOnly = false;
            bool isDefaultOnly = false;
            for (auto anno : annoList) {
                if (anno->name == IR::Annotation::tableOnlyAnnotation) {
                    isTableOnly = true;
                }
                if (anno->name == IR::Annotation::defaultOnlyAnnotation) {
                    isDefaultOnly = true;
                }
                auto actionAnno = new struct Annotation(anno->name);
                actionInfo->annotations.push_back(actionAnno);
            }
            if (isTableOnly && isDefaultOnly) {
                ::error(
                    "Table '%1%' has an action reference '%2%' which is "
                    "annotated with both '@tableonly' and '@defaultonly'",
                    p4table->getName().originalName, action->getName().originalName);
            } else if (isTableOnly) {
                actionInfo->scope = TableOnly;
            } else if (isDefaultOnly) {
                actionInfo->scope = DefaultOnly;
            }
            if ((table->defaultHitAction != nullptr) &&
                (table->defaultHitAction->actionName == actionInfo->name)) {
                actionInfo->defaultHit = true;
            }
            if ((table->defaultMissAction != nullptr) &&
                (table->defaultMissAction->actionName == actionInfo->name)) {
                actionInfo->defaultMiss = true;
            }
            unsigned int id = 1;
            for (auto actParam : actionDef->actionParams) {
                auto param = new struct ActionParam();
                param->id = id++;
                param->name = actParam->paramName;
                param->dataType = actParam->dataType;
                param->bitwidth = actParam->bitSize;
                actionInfo->actionParams.push_back(param);
            }
            tableInfo->actions.push_back(actionInfo);
        }
    }
}

void IntrospectionGenerator::genTableJson(Util::JsonArray *tablesJson) {
    for (auto table : tablesInfo) {
        auto tableJson = genTableInfo(table);
        tablesJson->append(tableJson);
    }
}

Util::JsonObject *IntrospectionGenerator::genActionInfo(struct ActionAttributes *action) {
    auto actionJson = new Util::JsonObject();
    actionJson->emplace("id", action->id);
    actionJson->emplace("name", action->name);
    cstring actionScope = "";
    if (action->scope == TableOnly) {
        actionScope = "TableOnly";
    } else if (action->scope == DefaultOnly) {
        actionScope = "DefaultOnly";
    } else {
        actionScope = "TableAndDefault";
    }
    actionJson->emplace("action_scope", actionScope);
    auto annoArray = new Util::JsonArray();
    for (auto anno : action->annotations) {
        annoArray->append(anno->name);
    }
    actionJson->emplace("annotations", annoArray);
    auto paramArray = new Util::JsonArray();
    for (auto param : action->actionParams) {
        auto paramJson = new Util::JsonObject();
        paramJson->emplace("id", param->id);
        paramJson->emplace("name", param->name);
        switch (param->dataType) {
            case TC::BIT_TYPE: {
                paramJson->emplace("type", "bit");
                break;
            }
            case TC::DEV_TYPE: {
                paramJson->emplace("type", "dev");
                break;
            }
            case TC::MACADDR_TYPE: {
                paramJson->emplace("type", "macaddr");
                break;
            }
            case TC::IPV4_TYPE: {
                paramJson->emplace("type", "ipv4");
                break;
            }
            case TC::IPV6_TYPE: {
                paramJson->emplace("type", "ipv6");
                break;
            }
            case TC::BE16_TYPE: {
                paramJson->emplace("type", "be16");
                break;
            }
            case TC::BE32_TYPE: {
                paramJson->emplace("type", "be32");
                break;
            }
            case TC::BE64_TYPE: {
                paramJson->emplace("type", "be64");
                break;
            }
        }
        paramJson->emplace("bitwidth", param->bitwidth);
        paramArray->append(paramJson);
    }
    actionJson->emplace("params", paramArray);
    actionJson->emplace("default_hit_action", action->defaultHit);
    actionJson->emplace("default_miss_action", action->defaultMiss);
    return actionJson;
}

Util::JsonObject *IntrospectionGenerator::genKeyInfo(struct KeyFieldAttributes *keyField) {
    auto keyJson = new Util::JsonObject();
    keyJson->emplace("id", keyField->id);
    keyJson->emplace("name", keyField->name);
    keyJson->emplace("type", keyField->type);
    keyJson->emplace("match_type", keyField->matchType);
    keyJson->emplace("bitwidth", keyField->bitwidth);
    return keyJson;
}

Util::JsonObject *IntrospectionGenerator::genTableInfo(struct TableAttributes *tbl) {
    auto tableJson = new Util::JsonObject();
    tableJson->emplace("name", tbl->name);
    tableJson->emplace("id", tbl->id);
    tableJson->emplace("tentries", tbl->tentries);
    tableJson->emplace("nummask", tbl->numMask);
    if (tbl->keysize != 0) {
        tableJson->emplace("keysize", tbl->keysize);
    }
    auto keysJson = new Util::JsonArray();
    for (auto keyField : tbl->keyFields) {
        auto keyJson = genKeyInfo(keyField);
        keysJson->append(keyJson);
    }
    if (keysJson->size() != 0) {
        tableJson->emplace("keyfields", keysJson);
    }
    auto actionsJson = new Util::JsonArray();
    for (auto action : tbl->actions) {
        auto actionJson = genActionInfo(action);
        actionsJson->append(actionJson);
    }
    if (actionsJson->size() != 0) {
        tableJson->emplace("actions", actionsJson);
    }
    return tableJson;
}

const Util::JsonObject *IntrospectionGenerator::genIntrospectionJson() {
    auto *json = new Util::JsonObject();
    auto *tablesJson = new Util::JsonArray();
    struct IntrospectionInfo introspec;
    collectTableInfo();
    introspec.initIntrospectionInfo(tcPipeline);
    json->emplace("schema_version", introspec.schemaVersion);
    json->emplace("pipeline_name", introspec.pipelineName);
    json->emplace("id", introspec.pipelineId);
    genTableJson(tablesJson);
    json->emplace("tables", tablesJson);
    return json;
}

bool IntrospectionGenerator::serializeIntrospectionJson(std::ostream &destination) {
    auto *json = genIntrospectionJson();
    if (::errorCount() > 0) {
        return false;
    }
    json->serialize(destination);
    return true;
}

std::optional<cstring> IntrospectionGenerator::checkValidTcType(const IR::StringLiteral *sl) {
    auto value = sl->value;
    if (value == "dev" || value == "macaddr" || value == "ipv4" || value == "ipv6" ||
        value == "be16" || value == "be32" || value == "be64") {
        return value;
    }
    return std::nullopt;
}

}  // namespace TC
