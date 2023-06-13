/*
Copyright 2021 Intel Corp.

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

#include "dpdkContext.h"

#include "backend.h"
#include "control-plane/bfruntime_ext.h"
#include "dpdkUtils.h"
#include "printUtils.h"
namespace DPDK {

cstring DpdkContextGenerator::removePipePrefix(cstring tableName) {
    if (!options.bfRtSchema.isNullOrEmpty() || !options.tdiFile.isNullOrEmpty()) {
        cstring tablename = tableName.find('.');
        tablename = tablename.trim(".\t\n\r");
        return tablename;
    }
    return tableName;
}

// This function collects all tables in a vector and sets the table attributes required
// by context json into a map.
void DpdkContextGenerator::CollectTablesAndSetAttributes() {
    IR::IndexedVector<IR::Declaration> selector_tables;
    IR::IndexedVector<IR::Declaration> action_data_tables;
    for (auto kv : structure->pipelines) {
        cstring direction = "";
        if (kv.first == "Ingress")
            direction = "ingress";
        else if (kv.first == "Egress")
            direction = "egress";
        auto control = kv.second->to<IR::P4Control>();
        for (auto d : control->controlLocals) {
            if (auto tbl = d->to<IR::P4Table>()) {
                struct TableAttributes tblAttr = {};
                tblAttr.direction = direction;
                tblAttr.controlName = control->name.originalName;
                tblAttr.externalName = tbl->controlPlaneName();
                tblAttr.tableHandle = getHandleId(tblAttr.externalName);
                auto size = tbl->getSizeProperty();
                tblAttr.size = dpdk_default_table_size;
                if (size) tblAttr.size = size->asUnsigned();
                auto hidden = tbl->annotations->getSingle(IR::Annotation::hiddenAnnotation);
                auto selector = tbl->properties->getProperty("selector");
                tblAttr.is_add_on_miss = false;
                tblAttr.idle_timeout_with_auto_delete = false;
                auto add_on_miss = tbl->properties->getProperty("add_on_miss");
                if (add_on_miss != nullptr) {
                    if (add_on_miss->value->is<IR::ExpressionValue>()) {
                        auto expr = add_on_miss->value->to<IR::ExpressionValue>()->expression;
                        if (!expr->is<IR::BoolLiteral>()) {
                            ::error(ErrorType::ERR_UNEXPECTED,
                                    "%1%: expected boolean for 'add_on_miss' property",
                                    add_on_miss);
                            return;
                        } else {
                            tblAttr.is_add_on_miss = expr->to<IR::BoolLiteral>()->value;
                        }
                    }
                }
                auto idle_timeout_with_auto_delete =
                    tbl->properties->getProperty("idle_timeout_with_auto_delete");
                if (idle_timeout_with_auto_delete != nullptr) {
                    if (idle_timeout_with_auto_delete->value->is<IR::ExpressionValue>()) {
                        auto expr = idle_timeout_with_auto_delete->value->to<IR::ExpressionValue>()
                                        ->expression;
                        if (!expr->is<IR::BoolLiteral>()) {
                            ::error(ErrorType::ERR_UNEXPECTED,
                                    "%1%: expected boolean for 'idle_timeout_with_auto_delete' "
                                    "property",
                                    idle_timeout_with_auto_delete);
                            return;
                        } else {
                            tblAttr.idle_timeout_with_auto_delete =
                                expr->to<IR::BoolLiteral>()->value;
                        }
                    }
                }
                if (hidden) {
                    tblAttr.tableType = selector ? "selection" : "action";
                    tblAttr.isHidden = true;
                } else {
                    tblAttr.isHidden = false;
                    tblAttr.tableType = "match";
                    tblAttr.tableKeys = ::get(structure->key_map, kv.second->name.originalName +
                                                                      "_" + tbl->name.originalName);
                }
                if (!hidden)
                    tables.push_back(d);
                else if (!selector)
                    action_data_tables.push_back(d);
                else
                    selector_tables.push_back(d);
                tableAttrmap.emplace(tbl->name.originalName, tblAttr);
            }
        }
    }

    for (auto ed : structure->externDecls) {
        cstring externTypeName = "";
        if (auto type = ed->type->to<IR::Type_Name>()) {
            externTypeName = type->path->name.name;
        } else if (auto type = ed->type->to<IR::Type_Specialized>()) {
            externTypeName = type->baseType->path->name.name;
        }
        if (externTypeName == "Counter" || externTypeName == "DirectCounter" ||
            externTypeName == "Register" || externTypeName == "DirectMeter" ||
            externTypeName == "Meter" || externTypeName == "Hash" ||
            externTypeName == "InternetCheckSum") {
            struct externAttributes externAttr;
            externAttr.externalName = ed->controlPlaneName();
            externAttr.externType = externTypeName;
            if (externTypeName == "Counter" || externTypeName == "DirectCounter") {
                unsigned maxArgNum = externTypeName == "Counter" ? 2 : 1;
                int typeArgNum = maxArgNum - 1;
                if (ed->arguments->size() != maxArgNum) {
                    ::error(ErrorType::ERR_UNEXPECTED,
                            "%1%: expected %2% arguments, number of counters and type"
                            "of counter",
                            ed, maxArgNum);
                }
                auto counter_type = ed->arguments->at(typeArgNum)->expression;
                BUG_CHECK(counter_type->is<IR::Constant>(), "Expected counter type to be constant");
                auto value = counter_type->to<IR::Constant>()->asUnsigned();
                switch (value) {
                    case 0:
                        externAttr.counterType = "packets";
                        break;
                    case 1:
                        externAttr.counterType = "bytes";
                        break;
                    case 2:
                        externAttr.counterType = "packets_and_bytes";
                        break;
                }
            }
            if (externTypeName == "DirectMeter" || externTypeName == "DirectCounter") {
                auto ownerTable = ::get(structure->direct_resource_map, ed->name.name);
                auto tableAttr = ::get(tableAttrmap, ownerTable->name.originalName);
                externAttr.table_id = tableAttr.tableHandle;
            }
            externAttrMap.emplace(ed->name.name, externAttr);
            externs.push_back(ed);
        }
    }

    // Keep the compiler generated (hidden) tables at the end
    for (auto d : selector_tables) tables.push_back(d);
    for (auto d : action_data_tables) tables.push_back(d);
}

// This functions insert a single key field in the match keys array
void DpdkContextGenerator::addKeyField(Util::JsonArray *keyJson, const cstring name,
                                       const cstring nameAnnotation, const IR::KeyElement *key,
                                       int position) {
    auto *keyField = new Util::JsonObject();
    cstring fieldName = name.findlast('.');
    auto instanceName = name.replace(fieldName, "");
    fieldName = fieldName.trim(".\t\n\r");
    std::string keyName(nameAnnotation);
    // Replace header stack indices hdr[<index>] with hdr$<index>.
    std::regex hdrStackRegex(R"(\[([0-9]+)\])");
    keyName = std::regex_replace(keyName, hdrStackRegex, "$$$1");
    keyField->emplace("name", keyName);
    keyField->emplace("instance_name", instanceName);
    keyField->emplace("field_name", fieldName);
    auto match_kind = toStr(key->matchType);
    if (match_kind == "optional" || match_kind == "range") match_kind = "ternary";
    keyField->emplace("match_type", match_kind);
    keyField->emplace("start_bit", 0);
    keyField->emplace("bit_width", key->expression->type->width_bits());
    keyField->emplace("bit_width_full", key->expression->type->width_bits());
    keyField->emplace("position", position);
    keyJson->append(keyField);
}

// This function sets the common table properties
Util::JsonObject *DpdkContextGenerator::initTableCommonJson(const cstring name,
                                                            const struct TableAttributes &attr) {
    auto *tableJson = new Util::JsonObject();
    cstring tableName = name;
    tableJson->emplace("name", attr.externalName);
    tableJson->emplace("target_name", tableName);
    tableJson->emplace("direction", attr.direction);
    tableJson->emplace("handle", attr.tableHandle);
    tableJson->emplace("table_type", attr.tableType);
    tableJson->emplace("size", attr.size);
    tableJson->emplace("p4_hidden", attr.isHidden);
    tableJson->emplace("add_on_miss", attr.is_add_on_miss);
    tableJson->emplace("idle_timeout_with_auto_delete", attr.idle_timeout_with_auto_delete);
    return tableJson;
}

void DpdkContextGenerator::collectHandleId() {
    for (auto &table : p4info.tables()) {
        const auto &pre_t = table.preamble();
        context_handle_map[removePipePrefix(pre_t.name())] = pre_t.id();
    }
    for (auto &action : p4info.actions()) {
        const auto &pre_a = action.preamble();
        context_handle_map[pre_a.name()] = pre_a.id();
    }
    for (auto &action_prof : p4info.action_profiles()) {
        const auto &pre_a = action_prof.preamble();
        context_handle_map[removePipePrefix(pre_a.name())] = pre_a.id();
    }
    for (const auto &externType : p4info.externs()) {
        for (const auto &externInstance : externType.instances()) {
            const auto &pre_a = externInstance.preamble();
            context_handle_map[removePipePrefix(pre_a.name())] = pre_a.id();
        }
    }
}

size_t DpdkContextGenerator::getHandleId(cstring name) {
    size_t id = 0;
    if (context_handle_map.count(name)) id = context_handle_map[name];
    BUG_CHECK(id != 0, "unable to find id for %1%", name);
    return id;
}

// This function sets action attributes for actions in a table.
void DpdkContextGenerator::setActionAttributes(const IR::P4Table *tbl) {
    for (auto act : tbl->getActionList()->actionList) {
        struct actionAttributes attr;
        auto action_decl = refmap->getDeclaration(act->getPath())->to<IR::P4Action>();
        bool has_constant_default_action = false;
        auto prop = tbl->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
        if (prop && prop->isConstant) has_constant_default_action = true;
        bool is_const_default_action = false;
        bool can_be_hit_action = true;
        bool is_compiler_added_action = false;
        bool can_be_default_action = !has_constant_default_action;

        // First, check for action annotations
        auto actAnnot = action_decl->annotations;
        auto table_only_annot = actAnnot->getSingle(IR::Annotation::tableOnlyAnnotation);
        auto default_only_annot = actAnnot->getSingle(IR::Annotation::defaultOnlyAnnotation);
        auto hidden = actAnnot->getSingle(IR::Annotation::hiddenAnnotation);

        if (table_only_annot) {
            can_be_default_action = false;
        }
        if (default_only_annot) {
            can_be_hit_action = false;
        }
        if (hidden) {
            is_compiler_added_action = true;
        }

        // Second, see if this action is the default action and/or constant default action
        auto default_action = tbl->getDefaultAction();
        if (default_action) {
            if (auto mc = default_action->to<IR::MethodCallExpression>()) {
                default_action = mc->method;
            }

            auto path = default_action->to<IR::PathExpression>();
            BUG_CHECK(path, "Default action path %s cannot be found", default_action);
            auto actName = refmap->getDeclaration(path->path, true)->getName();

            if (actName == act->getName()) {
                if (has_constant_default_action) {
                    can_be_default_action = true;  // this is the constant default action
                    is_const_default_action = true;
                }
            }
        }

        /* DPDK target takes a structure as parameter for Actions. So, all action
           parameters are collected into a structure by an earlier pass. */
        auto params = ::get(structure->args_struct_map, act->getPath()->name.name + "_arg_t");
        if (params)
            attr.params = params->clone();
        else
            attr.params = nullptr;

        attr.constant_default_action = is_const_default_action;
        attr.is_compiler_added_action = is_compiler_added_action;
        attr.allowed_as_hit_action = can_be_hit_action;
        attr.allowed_as_default_action = can_be_default_action;
        attr.actionHandle = getHandleId(action_decl->controlPlaneName());
        attr.externalName = action_decl->controlPlaneName();
        actionAttrMap.emplace(act->getName(), attr);
    }
}

// This functions updates the table attribute map entry with default action handle
// for the specified table.
void DpdkContextGenerator::setDefaultActionHandle(const IR::P4Table *table) {
    cstring default_action_name = "";
    if (table->getDefaultAction()) default_action_name = toStr(table->getDefaultAction());

    auto tableAttr = ::get(tableAttrmap, table->name.originalName);
    for (auto action : table->getActionList()->actionList) {
        struct actionAttributes attr = ::get(actionAttrMap, action->getName());
        if (toStr(action->expression) == default_action_name) {
            tableAttr.default_action_handle = attr.actionHandle;
            // Update default table handle in existing table attribute map
            tableAttrmap.find(table->name.originalName)->second = tableAttr;
            break;
        }
    }
}

// This functions creates JSON object for immediate fields (action parameters)
void DpdkContextGenerator::addImmediateField(Util::JsonArray *paramJson, const cstring name,
                                             int dest_start, int dest_width) {
    auto *oneParam = new Util::JsonObject();
    oneParam->emplace("param_name", name);
    oneParam->emplace("dest_start", dest_start);
    oneParam->emplace("dest_width", dest_width);
    paramJson->append(oneParam);
}

// This functions creates JSON object for match attributes of a table.
Util::JsonObject *DpdkContextGenerator::addMatchAttributes(const IR::P4Table *table,
                                                           const cstring ctrlName) {
    auto tableAttr = ::get(tableAttrmap, table->name.originalName);
    auto *match_attributes = new Util::JsonObject();
    auto *actFmtArray = new Util::JsonArray();
    auto *stageTblArray = new Util::JsonArray();
    auto *oneStageTbl = new Util::JsonObject();
    for (auto action : table->getActionList()->actionList) {
        auto *oneAction = new Util::JsonObject();
        struct actionAttributes attr = ::get(actionAttrMap, action->getName());
        auto name = action->externalName();
        if (name != "NoAction") {
            name = ctrlName + "." + name;
        }
        oneAction->emplace("action_name", name);
        oneAction->emplace("action_handle", attr.actionHandle);
        auto *immFldArray = new Util::JsonArray();
        if (attr.params) {
            int index = 0;
            int param_width = 8;  // Minimum width for dpdk action params
            for (auto param : *(attr.params)) {
                if (param->type->is<IR::Type_Bits>()) {
                    param_width = param->type->width_bits();
                    param_width = getMetadataFieldWidth(param_width);
                } else if (!param->type->is<IR::Type_Boolean>()) {
                    BUG("Unsupported parameter type %1%", param->type);
                }
                addImmediateField(immFldArray, param->name.originalName, index / 8, param_width);
                index += param_width;
            }
        }
        oneAction->emplace("immediate_fields", immFldArray);
        actFmtArray->append(oneAction);
    }
    oneStageTbl->emplace("action_format", actFmtArray);
    stageTblArray->append(oneStageTbl);
    match_attributes->emplace("stage_tables", stageTblArray);
    return match_attributes;
}

// This function adds a single parameter to the parameters array.
void DpdkContextGenerator::addActionParam(Util::JsonArray *paramJson, const cstring name,
                                          int bitWidth, int position, int byte_array_index) {
    auto *oneParam = new Util::JsonObject();
    oneParam->emplace("name", name);
    oneParam->emplace("start_bit", 0);
    oneParam->emplace("bit_width", bitWidth);
    oneParam->emplace("position", position);
    oneParam->emplace("byte_array_index", byte_array_index);
    paramJson->append(oneParam);
}

// This function creates JSON objects for  actions within a table.
Util::JsonArray *DpdkContextGenerator::addActions(const IR::P4Table *table,
                                                  const cstring controlName, bool isMatch) {
    auto *actArray = new Util::JsonArray();
    for (auto action : table->getActionList()->actionList) {
        struct actionAttributes attr = ::get(actionAttrMap, action->getName());
        // Printing compiler added actions is curently not required
        if (!attr.is_compiler_added_action) {
            auto *act = new Util::JsonObject();
            auto actName = toStr(action->expression);
            auto name = action->externalName();
            if (name != "NoAction") {
                actName = controlName + "." + actName;
            } else {
                actName = name;
            }
            act->emplace("name", attr.externalName);
            act->emplace("target_name", actName);
            act->emplace("handle", attr.actionHandle);
            if (isMatch) {
                act->emplace("constant_default_action", attr.constant_default_action);
                act->emplace("is_compiler_added_action", attr.is_compiler_added_action);
                act->emplace("allowed_as_hit_action", attr.allowed_as_hit_action);
                act->emplace("allowed_as_default_action", attr.allowed_as_default_action);
                ;
            }
            auto *paramJson = new Util::JsonArray();
            if (attr.params) {
                int index = 0;
                int position = 0;
                int param_width = 8;
                for (auto param : *(attr.params)) {
                    if (param->type->is<IR::Type_Bits>()) {
                        param_width = param->type->width_bits();
                        param_width = getMetadataFieldWidth(param_width);
                    } else if (!param->type->is<IR::Type_Boolean>()) {
                        BUG("Unsupported parameter type %1%", param->type);
                    }
                    addActionParam(paramJson, param->name.originalName, param_width, position,
                                   index / 8);
                    index += param_width;
                    position++;
                }
            }
            act->emplace("p4_parameters", paramJson);
            actArray->append(act);
        }
    }
    return actArray;
}

// This function adds the tables referred by this table.
bool DpdkContextGenerator::addRefTables(const cstring tbl_name, const IR::P4Table **memberTable,
                                        Util::JsonObject *tableJson) {
    bool hasActionProfileSelector = false;

    // Below empty arrays are currently required by the control plane software.
    // May be removed in future.
    tableJson->emplace("stateful_table_refs", new Util::JsonArray());
    tableJson->emplace("statistics_table_refs", new Util::JsonArray());
    tableJson->emplace("meter_table_refs", new Util::JsonArray());

    // Reference to compiler generated member table in case of action profile and action selector.
    if (structure->member_tables.count(tbl_name)) {
        auto *actionDataJson = new Util::JsonArray();
        hasActionProfileSelector = true;
        *memberTable = structure->member_tables.at(tbl_name);
        auto *actionDataField = new Util::JsonObject();
        auto tableAttr = ::get(tableAttrmap, (*memberTable)->name.originalName);
        auto tableName = tableAttr.controlName + "." + (*memberTable)->name.originalName;
        actionDataField->emplace("name", tableName);
        actionDataField->emplace("handle", tableAttr.tableHandle);
        actionDataJson->append(actionDataField);
        tableJson->emplace("action_data_table_refs", actionDataJson);
    }

    // Reference to compiler generated group table in case of action selector
    if (structure->group_tables.count(tbl_name)) {
        auto *selectionJson = new Util::JsonArray();
        hasActionProfileSelector = true;
        auto groupTable = structure->group_tables.at(tbl_name);
        auto *selectField = new Util::JsonObject();
        auto tableAttr = ::get(tableAttrmap, groupTable->name.originalName);
        auto tableName = tableAttr.controlName + "." + groupTable->name.originalName;
        selectField->emplace("name", tableName);
        selectField->emplace("handle", tableAttr.tableHandle);
        selectionJson->append(selectField);
        tableJson->emplace("selection_table_refs", selectionJson);
    }

    if (hasActionProfileSelector) {
        tableJson->emplace("action_profile", (*memberTable)->name.originalName);
    }
    return hasActionProfileSelector;
}

// Add tables to the context json
void DpdkContextGenerator::addMatchTables(Util::JsonArray *tablesJson) {
    for (auto t : tables) {
        auto tbl = t->to<IR::P4Table>();
        auto tableAttr = ::get(tableAttrmap, tbl->name.originalName);
        auto *tableJson = initTableCommonJson(tbl->name.originalName, tableAttr);
        bool hasActionProfileSelector = false;
        bool isMatchTable = tableAttr.tableType == "match";
        const IR::P4Table *memberTable = nullptr;
        if (tableAttr.tableType != "selection") {
            if (isMatchTable) {
                hasActionProfileSelector = addRefTables(tbl->name, &memberTable, tableJson);
                auto match_keys = tbl->getKey();
                if (match_keys) {
                    auto *keyJson = new Util::JsonArray();
                    int position = 0;
                    for (auto matchKeyFromPrg : tableAttr.tableKeys) {
                        addKeyField(keyJson, matchKeyFromPrg.first, matchKeyFromPrg.second,
                                    match_keys->keyElements.at(position), position);
                        position++;
                    }
                    tableJson->emplace("match_key_fields", keyJson);
                }
            }
            // If table implementation is action profile or action selector, all actions from member
            // table should be output for the base table.
            const IR::P4Table *table = nullptr;
            if (hasActionProfileSelector) {
                table = memberTable;
            } else {
                table = tbl;
            }

            setActionAttributes(table);
            setDefaultActionHandle(table);

            tableAttr = ::get(tableAttrmap, table->name.originalName);
            tableJson->emplace("actions", addActions(table, tableAttr.controlName, isMatchTable));
            if (isMatchTable) {
                tableJson->emplace("match_attributes",
                                   addMatchAttributes(table, tableAttr.controlName));
            }
            tableJson->emplace("default_action_handle", tableAttr.default_action_handle);
        } else {
            SelectionTable sel;
            sel.setAttributes(tbl, tableAttrmap);
            tableJson->emplace("max_n_groups", sel.max_n_groups);
            tableJson->emplace("max_n_members_per_group", sel.max_n_members_per_group);
            tableJson->emplace("bound_to_action_data_table_handle",
                               sel.bound_to_action_data_table_handle);
        }
        tablesJson->append(tableJson);
    }
}

// Add extern information to the context json
void DpdkContextGenerator::addExternInfo(Util::JsonArray *externsJson) {
    for (auto t : externs) {
        auto externAttr = ::get(externAttrMap, t->name.name);
        auto *externJson = new Util::JsonObject();
        externJson->emplace("name", externAttr.externalName);
        externJson->emplace("target_name", t->name.name);
        externJson->emplace("type", externAttr.externType);
        auto *attrJson = new Util::JsonObject();
        if (externAttr.externType == "Counter" || externAttr.externType == "DirectCounter") {
            attrJson->emplace("type", externAttr.counterType);
        }
        if (externAttr.externType == "DirectCounter" || externAttr.externType == "DirectMeter") {
            attrJson->emplace("table_id", externAttr.table_id);
        }
        externJson->emplace("attributes", attrJson);
        externsJson->append(externJson);
    }
}

const Util::JsonObject *DpdkContextGenerator::genContextJsonObject() {
    auto *json = new Util::JsonObject();
    auto *tablesJson = new Util::JsonArray();
    auto *externsJson = new Util::JsonArray();
    struct TopLevelCtxt tlinfo;
    tlinfo.initTopLevelCtxt(options);
    json->emplace("program_name", tlinfo.progName);
    json->emplace("build_date", tlinfo.buildDate);
    json->emplace("compile_command", tlinfo.compileCommand);
    json->emplace("compiler_version", tlinfo.compilerVersion);
    json->emplace("schema_version", cstring("0.1"));
    json->emplace("target", cstring("DPDK"));
    json->emplace("tables", tablesJson);
    addMatchTables(tablesJson);
    json->emplace("externs", externsJson);
    addExternInfo(externsJson);
    return json;
}

void DpdkContextGenerator::serializeContextJson(std::ostream *destination) {
    collectHandleId();
    CollectTablesAndSetAttributes();
    auto *json = genContextJsonObject();
    json->serialize(*destination);
    destination->flush();
}

}  // namespace DPDK
