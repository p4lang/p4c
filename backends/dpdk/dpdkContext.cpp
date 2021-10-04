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
#include "printUtils.h"

namespace DPDK {

/* Unique handle for action and table */
#define TABLE_HANDLE_PREFIX 0x00010000
#define ACTION_HANDLE_PREFIX 0x00020000

unsigned CollectTablesForContextJson::newTableHandle = 0;

/* This function sets certain attributes for each table */
void CollectTablesForContextJson::setTableAttributes() {
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
                struct TableAttributes tblAttr;
                tblAttr.direction = direction;
                tblAttr.controlName = control->name.originalName;
                tblAttr.tableHandle = getNewTableHandle();
                auto hidden = tbl->annotations->getSingle(IR::Annotation::hiddenAnnotation);
                auto selector = tbl->properties->getProperty("selector");
                if (hidden) {
                    tblAttr.tableType = selector ? "selection" : "action";
                    tblAttr.isHidden = true;
                } else {
                    tblAttr.isHidden = false;
                    tblAttr.tableType = "match";
                    tblAttr.tableKeys = ::get (structure->key_map, kv.second->name.originalName
                                               + "_" + tbl->name.originalName);
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

    // Keep the compiler generated (hidden) tables at the end
    for (auto d : selector_tables)
        tables.push_back(d);
    for (auto d : action_data_tables)
        tables.push_back(d);
}

bool CollectTablesForContextJson::preorder(const IR::P4Program * /*p*/) {
        setTableAttributes();
        return false;
}

// Returns a unique ID for table
unsigned int CollectTablesForContextJson::getNewTableHandle() {
        return TABLE_HANDLE_PREFIX | newTableHandle++;
}

unsigned WriteContextJson::newActionHandle = 0;

// Returns a unique ID for action
unsigned int WriteContextJson::getNewActionHandle() {
        return ACTION_HANDLE_PREFIX | newActionHandle++;
}

// Helper function for pretty printing into JSON file
void WriteContextJson::add_space(std::ostream &out, int size) {
    out << std::setfill(' ') << std::setw(size) << " ";
}

/* Main function for generating context JSON */
bool WriteContextJson::preorder(const IR::P4Program * /*p*/) {
    if (!options.ctxtFile.isNullOrEmpty()) {
        std::ostream *outCtxt = openFile(options.ctxtFile, false);
        if (outCtxt != nullptr) {
            std::ostream &out = *outCtxt;

            /* Fetch required information from options */
            const time_t now = time(NULL);
            char build_date[50];
            strftime(build_date, 50, "%c", localtime(&now));
            cstring compilerCommand = options.DpdkCompCmd;
            compilerCommand = compilerCommand.replace("(from pragmas)", "");
            compilerCommand = compilerCommand.trim();
            cstring progName =  options.file;
            progName = progName.findlast('/');
            auto fileext = progName.find(".");
            progName = progName.replace(fileext, "");
            progName = progName.trim("/\t\n\r");

            /* Print program level information*/
            out << "{\n";
            add_space(out, 4); out << "\"program_name\": \"" << progName <<"\",\n";
            add_space(out, 4); out << "\"build_date\": \"" << build_date << "\",\n";
            add_space(out, 4); out << "\"compile_command\": \"" << compilerCommand << " \",\n";
            add_space(out, 4); out << "\"compiler_version\": \"" << options.compilerVersion << "\",\n";
            add_space(out, 4); out << "\"schema_version\": \"0.1\",\n";
            add_space(out, 4); out << "\"target\": \"DPDK\",\n";

            /* Table array starts here */
            add_space(out, 4); out << "\"tables\": [";
            bool first = true;
            for (auto t : tables) {
                if (!first) out << ",";
                out << "\n";
                auto tbl = t->to<IR::P4Table>();
                auto tableAttr = ::get(tableAttrmap, tbl->name.originalName);
                if (tableAttr.tableType == "match")
                    printTableCtxtJson(tbl, out);
                if (tableAttr.tableType == "selection")
                    printSelTableCtxtJson(tbl, out);
                else if (tableAttr.tableType == "action")
                    printActionTableCtxtJson(tbl, out);
                first = false;
            }
            if (!first) {
                out << "\n"; add_space(out, 4);
            }
            out << "]\n";
            out << "}\n";
            outCtxt->flush();
        }
    }
    return false;
}

/* This function sets certain attributes for each action within a table */
void WriteContextJson::setActionAttributes (
    std::map <cstring, struct actionAttributes> &actionAttrMap, const IR::P4Table *tbl) {
    for (auto act : tbl->getActionList()->actionList) {
        struct actionAttributes attr;
        auto action_decl = refmap->getDeclaration(act->getPath())->to<IR::P4Action>();
        bool has_constant_default_action = false;
        auto prop = tbl->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
        if (prop && prop->isConstant)
            has_constant_default_action = true;
        bool is_const_default_action = false;
        bool can_be_hit_action = true;
        bool is_compiler_added_action = false;
        bool can_be_default_action = !has_constant_default_action;

        // First, check for action annotations
        auto table_only_annot = action_decl->annotations->getSingle("tableonly");
        auto default_only_annot = action_decl->annotations->getSingle("defaultonly");
        auto hidden = action_decl->annotations->getSingle(IR::Annotation::hiddenAnnotation);

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
                if (!path)
                    BUG("Default action path %s cannot be found", default_action);
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
        auto params = ::get(structure->args_struct_map, act->externalName() + "_arg_t");
        if (params)
            attr.params = params->clone();
        else
            attr.params = nullptr;

        attr.constant_default_action = is_const_default_action;
        attr.is_compiler_added_action = is_compiler_added_action;
        attr.allowed_as_hit_action = can_be_hit_action;
        attr.allowed_as_default_action = can_be_default_action;
        attr.actionHandle = getNewActionHandle();
        actionAttrMap.emplace(act->getName(), attr);
    }
}

/* Print a single table object of match_table type in Context JSON*/
void WriteContextJson::printTableCtxtJson (const IR::P4Table *tbl, std::ostream &out) {
    bool hasActionProfileSelector = false;
    auto mainTableAttr = ::get(tableAttrmap, tbl->name.originalName);
    cstring tableName = mainTableAttr.controlName + "." + tbl->name.originalName;
    add_space(out, 8); out << "{\n";
    add_space(out, 12); out << "\"table_type\": \"" << mainTableAttr.tableType << "\",\n";
    add_space(out, 12); out << "\"direction\": \"" << mainTableAttr.direction << "\",\n";
    add_space(out, 12); out << "\"handle\": " << mainTableAttr.tableHandle << ",\n";
    add_space(out, 12); out << "\"name\": \"" << tableName << "\",\n";
    add_space(out, 12); out << "\"size\": ";

    if (auto size = tbl->properties->getProperty("size")) {
        out << size->value << ",\n";
    } else {
        // default table size for DPDK
        out << "65536" << ",\n";
    }

    // Below empty arrays are currently required by the control plane software.
    // May be removed in future.
    add_space(out, 12); out << "\"stateful_table_refs\": []" << ",\n";
    add_space(out, 12); out << "\"statistics_table_refs\": []" << ",\n";
    add_space(out, 12); out << "\"meter_table_refs\": []" << ",\n";
    add_space(out, 12); out << "\"p4_hidden\": " << std::boolalpha << mainTableAttr.isHidden << ",\n";

    // Reference to compiler generated member table in case of action profile and action selector.
    add_space (out, 12); out << "\"action_data_table_refs\": [";
    const IR::P4Table *memberTable = nullptr;
    if (structure->member_tables.count(tbl->name)) {
        hasActionProfileSelector = true;
        memberTable = structure->member_tables.at(tbl->name);
        auto tableAttr = ::get(tableAttrmap, memberTable->name.originalName);
        tableName = tableAttr.controlName + "." + memberTable->name.originalName;
        out << "\n";
        add_space (out, 16); out << "{\n";
        add_space(out, 20);
        out << "\"name\": \"" <<  tableName <<"\",\n";
        add_space(out, 20); out << "\"handle\": " << tableAttr.tableHandle   <<"\n";
        add_space (out, 16); out << "}\n";
        add_space(out, 12);
    }
    out << "],\n";

    // Reference to compiler generated group table in case of action selector
    add_space (out, 12); out << "\"selection_table_refs\": [";
    if (structure->group_tables.count(tbl->name)) {
        hasActionProfileSelector = true;
        auto groupTable = structure->group_tables.at(tbl->name);
        auto tableAttr = ::get(tableAttrmap, groupTable->name.originalName);
        tableName = tableAttr.controlName + "." + groupTable->name.originalName;
        out << "\n";
        add_space (out, 16); out << "{\n";
        add_space(out, 20); out << "\"name\": \"" << tableName <<"\",\n";
        add_space(out, 20); out << "\"handle\": " <<  tableAttr.tableHandle <<"\n";
        add_space (out, 16); out << "}\n";
        add_space(out, 12);
    }
    out << "],\n";
    if (hasActionProfileSelector) {
        add_space (out, 12);
        out << "\"action_profile\": \"" << memberTable->name.originalName <<"\",\n";
    }

    // Match Key Fields for this table
    add_space (out, 12); out << "\"match_key_fields\": [";
    auto match_keys = tbl->getKey();

    if (match_keys) {
        int index = 0;
   /*     if (match_keys->keyElements.size() != mainTableAttr.tableKeys.size()) {
            std::cout << match_keys->keyElements.size() << " " <<  mainTableAttr.tableKeys.size() << std::endl;
            BUG("Inconsistent match keys between input program and transformed code");
        }*/
//        for (auto mkf : match_keys->keyElements) {
//            if (mkf->expression->is<IR::Member>()) {
          for (auto matchKeyFromPrg : mainTableAttr.tableKeys) {
                auto mkf = match_keys->keyElements.at(index);
//                auto matchKeyFromPrg = mainTableAttr.tableKeys.at(index);
               if (mkf->expression->is<IR::Member>()) {
                cstring fieldName = matchKeyFromPrg.findlast('.');
                auto instanceName = matchKeyFromPrg.replace(fieldName, "");
                fieldName = fieldName.trim(".\t\n\r");
                if (index) out << ",";
                out << "\n";
                add_space(out, 16); out << "{\n";
                add_space(out, 20); out << "\"name\": \"" <<  matchKeyFromPrg <<"\",\n";
                add_space(out, 20); out << "\"instance_name\": \"" <<  instanceName <<"\",\n";
                add_space(out, 20); out << "\"field_name\": \"" <<  fieldName <<"\",\n";
                add_space(out, 20); out << "\"match_type\": \"" <<  toStr(mkf->matchType) <<"\",\n";
                add_space(out, 20); out << "\"start_bit\": 0,\n";
                add_space(out, 20); out << "\"bit_width\": " << mkf->expression->type->width_bits() << ",\n";
                add_space(out, 20); out << "\"bit_width_full\": "  << mkf->expression->type->width_bits() << ",\n";
                add_space(out, 20); out << "\"position\":" << index << "\n";
                add_space(out, 16); out << "} ";
                index++;
            } else {
                // Match keys must be part of header or metadata structures
                BUG("%1%: invalid match key", mkf->expression);
            }
        }
        if (index) {
            out << "\n";
            add_space(out, 12);
        }
    }
    out << "],\n";

    // If table implementation is action profile or action selector, all actions from member
    // table should be output for the base table.
    const IR::P4Table *table = nullptr;
    if (hasActionProfileSelector) {
        table = memberTable;
    } else {
        table = tbl;
    }

    setActionAttributes (actionAttrMap, table);
    cstring default_action_name = "";

    unsigned default_action_handle = 0;
    bool first = true;
    if (table->getDefaultAction())
        default_action_name = toStr(table->getDefaultAction());

    // Print actions associated with this table
    add_space(out, 12); out << "\"actions\": [";
    for (auto action : table->getActionList()->actionList) {
        struct actionAttributes attr = ::get(actionAttrMap, action->getName());
        if (toStr(action->expression) == default_action_name)
            default_action_handle = attr.actionHandle;

        // Printing compiler added actions is curently not required
        if (!attr.is_compiler_added_action) {
            if (!first) out << ",";
            out << "\n";
            add_space(out, 16); out << "{\n";
            auto actName = toStr(action->expression);

            // NoAction is not prefixed with control block name
            if (actName != "NoAction")
                actName = mainTableAttr.controlName + "." + actName;
            add_space(out, 20); out << "\"name\": \"" << actName << "\",\n";
            add_space(out, 20); out << "\"handle\": " << attr.actionHandle << ",\n";
            add_space(out, 20); out << "\"constant_default_action\": " <<
                                       std::boolalpha << attr.constant_default_action << ",\n";
            add_space(out, 20); out << "\"is_compiler_added_action\": " <<
                                       std::boolalpha << attr.is_compiler_added_action  << ",\n";
            add_space(out, 20); out << "\"allowed_as_hit_action\": " <<
                                       std::boolalpha << attr.allowed_as_hit_action << ",\n";
            add_space(out, 20); out << "\"allowed_as_default_action\": " <<
                                       std::boolalpha << attr.allowed_as_default_action << ",\n";

            // Print action parameters
            add_space(out, 20); out << "\"p4_parameters\": [";
            int position = 0;
            if (attr.params) {
                int index = 0;
                for (auto param : *(attr.params)) {
                    if (position) out << ",";
                    out << "\n";
                    add_space(out, 24); out << "{\n";
                    add_space(out, 28); out << "\"name\": \"" << param->name.originalName << "\",\n";
                    add_space(out, 28); out << "\"start_bit\": 0,\n";
                    add_space(out, 28); out << "\"bit_width\": " << param->type->width_bits() << ",\n";
                    add_space(out, 28); out << "\"position\": " << position++ << ",\n";
                    add_space(out, 28); out << "\"byte_array_index\": "  << index/8 <<  "\n";
                    add_space(out, 24); out << "}";
                    index += param->type->width_bits();
                }
            }
            if (position) {
                out << "\n"; add_space(out, 20);
            }
            out << "]\n";
            add_space(out, 16); out << "}";
            first = false;
        }
    }
    if (!first) out << "\n";
    add_space(out, 12); out << "],\n";

    // These fields is similar to the actions array with slightly different information
    // These are currently required by the control plane software but may be removed in future.
    add_space(out, 12); out << "\"match_attributes\": {\n";
    add_space(out, 16); out << "\"stage_tables\": [\n";
    add_space(out, 20); out << "{\n";
    add_space(out, 24); out << "\"action_format\": [";
    first = true;

    for (auto action : table->getActionList()->actionList) {
        struct actionAttributes attr = ::get(actionAttrMap, action->getName());
//        if (toStr(action->expression) == default_action_name)
//            default_action_handle = attr.actionHandle;
        auto actName = toStr(action->expression);
        if (actName != "NoAction")
            actName = mainTableAttr.controlName + "." + actName;
        if (!first) out << ",";
        out << "\n";
        add_space(out, 28); out << "{\n";
        add_space(out, 20); out << "\"action_name\": \"" << actName << "\",\n";
        add_space(out, 32); out << "\"action_handle\": " << attr.actionHandle << ",\n";
        add_space(out, 32); out << "\"immediate_fields\": [";

        int position = 0;
        if (attr.params) {
            int index = 0;
            for (auto param : *(attr.params)) {
              if (position) out << ",";
              out << "\n";
              add_space(out, 36); out << "{\n";
              add_space(out, 40); out << "\"param_name\": \"" << param->name.originalName << "\",\n";
              add_space(out, 40); out << "\"dest_start\": "  << index/8 <<  ",\n";
              add_space(out, 40); out << "\"dest_width\": " << param->type->width_bits() << "\n";
              add_space(out, 36); out << "}";
              index += param->type->width_bits();
              position++;
            }
        }
        if (position) {
            out << "\n"; add_space(out, 32);
        }
        out << "]\n";
        add_space(out, 28); out << "}";
        first = false;
    }
    out << "\n";
    add_space(out, 24); out << "]\n";
    add_space(out, 20); out << "}\n";
    add_space(out, 16); out << "]\n";
    add_space(out, 12); out << "},\n";
    if (default_action_handle) {
        add_space(out, 12); out << "\"default_action_handle\": " << default_action_handle << "\n";
    }
    add_space(out, 8); out << "}";
}

/* Print a single selection table object in Context JSON */
void WriteContextJson::printSelTableCtxtJson (const IR::P4Table *tbl, std::ostream &out) {
    auto mainTableAttr = ::get(tableAttrmap, tbl->name.originalName);
    cstring tableName = mainTableAttr.controlName + "." + tbl->name.originalName;
    add_space(out, 8); out << "{\n";
    add_space(out, 12); out << "\"table_type\": \"" << mainTableAttr.tableType << "\",\n";
    add_space(out, 12); out << "\"direction\": \"" << mainTableAttr.direction << "\",\n";
    add_space(out, 12); out << "\"handle\": " << mainTableAttr.tableHandle << ",\n";
    add_space(out, 12); out << "\"name\": \"" <<  tableName << "\",\n";
    add_space(out, 12); out << "\"size\": ";

    if (auto size = tbl->properties->getProperty("size")) {
        out << size->value << ",\n";
    } else {
        // default table size for DPDK
        out << "65536" << ",\n";
    }
    add_space(out, 12); out << "\"p4_hidden\": " <<
                               std::boolalpha << mainTableAttr.isHidden << ",\n";

    add_space(out, 12); out << "\"max_n_groups\": ";
    if (auto n_groups = tbl->properties->getProperty("n_groups_max")) {
        out << n_groups->value << ",\n";
    } else {
        out << "0" << ",\n";
    }
    add_space(out, 12); out << "\"max_n_members_per_group\": ";
    if (auto n_members = tbl->properties->getProperty("n_members_per_group_max")) {
        out << n_members->value << ",\n";
    } else {
        out << "0" << ",\n";
    }

    // Fetch associated member table handle
    cstring actionDataTableName = tbl->name.originalName;
    actionDataTableName = actionDataTableName.replace("_sel", "");
    auto actionTableAttr = ::get(tableAttrmap, actionDataTableName);
    add_space(out, 12);
    out << "\"bound_to_action_data_table_handle\": " << actionTableAttr.tableHandle << "\n";
    add_space(out, 8); out << "}";
}

/* Print a single member table object in Context JSON */
void WriteContextJson::printActionTableCtxtJson (const IR::P4Table *tbl, std::ostream &out) {
    auto mainTableAttr = ::get(tableAttrmap, tbl->name.originalName);
    cstring tableName = mainTableAttr.controlName + "." + tbl->name.originalName;
    add_space(out, 8); out << "{\n";
    add_space(out, 12); out << "\"table_type\": \"" << mainTableAttr.tableType << "\",\n";
    add_space(out, 12); out << "\"direction\": \"" << mainTableAttr.direction << "\",\n";
    add_space(out, 12); out << "\"handle\": " << mainTableAttr.tableHandle << ",\n";
    add_space(out, 12); out << "\"name\": \"" <<  tableName << "\",\n";
    add_space(out, 12); out << "\"size\": ";

    if (auto size = tbl->properties->getProperty("size")) {
        out << size->value << ",\n";
    } else {
        // default table size for DPDK
        out << "65536" << ",\n";
    }
    add_space(out, 12); out << "\"p4_hidden\": " <<
                               std::boolalpha << mainTableAttr.isHidden << ",\n";

    cstring default_action_name = "";
    unsigned default_action_handle = 0;

    if (tbl->getDefaultAction())
        default_action_name = toStr(tbl->getDefaultAction());

    // Print actions associated with this table
    add_space(out, 12); out << "\"actions\": [";
    bool first = true;
    for (auto action : tbl->getActionList()->actionList) {
        struct actionAttributes attr = ::get(actionAttrMap, action->getName());
        if (toStr(action->expression) == default_action_name)
            default_action_handle = attr.actionHandle;
        auto actName = toStr(action->expression);
        if (actName != "NoAction")
            actName = mainTableAttr.controlName + "." + actName;
        if (!first) out << ",";
        out << "\n";
        add_space(out, 16); out << "{\n";
        add_space(out, 20); out << "\"name\": \"" << actName << "\",\n";
        add_space(out, 20); out << "\"handle\": " << attr.actionHandle << ",\n";

        // Print action parameters
        add_space(out, 20); out << "\"p4_parameters\": [";
        int position = 0;
        if (attr.params) {
            int index = 0;
            for (auto param : *(attr.params)) {
                if (position) out << ",";
                out << "\n";
                add_space(out, 24); out << "{\n";
                add_space(out, 28); out << "\"name\": \"" << param->name.originalName << "\",\n";
                add_space(out, 28); out << "\"start_bit\": 0,\n";
                add_space(out, 28); out << "\"bit_width\": " << param->type->width_bits() << ",\n";
                add_space(out, 28); out << "\"position\": " << position++ << ",\n";
                add_space(out, 28); out << "\"byte_array_index\": "  << index/8 <<  "\n";
                add_space(out, 24); out << "}";
                index += param->type->width_bits();
            }
        }
        if (position) {
            out << "\n"; add_space(out, 20);
        }
        out << "]\n";
        add_space(out, 16); out << "}";
        first = false;

    }
    if (!first) out << "\n";
    add_space(out, 12); out << "],\n";
    if (default_action_handle) {
        add_space(out, 12); out << "\"default_action_handle\": " << default_action_handle << "\n";
    }
    add_space(out, 8); out << "}";
}
}
