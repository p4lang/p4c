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

#ifndef BACKENDS_DPDK_DPDKCONTEXT_H_
#define BACKENDS_DPDK_DPDKCONTEXT_H_

#include <stddef.h>

#include <algorithm>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "control-plane/p4RuntimeSerializer.h"
#include "dpdkProgramStructure.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/json.h"
#include "lib/map.h"
#include "options.h"
#include "p4/config/v1/p4info.pb.h"

namespace p4configv1 = ::p4::config::v1;
/**
Passes defined in this file are used for generating Context JSON output for DPDK
Context JSON is a JSON file used by the control plane software for manipulating tables and
actions. It contains all relevant information regarding the tables and actions.
The context JSON is based on the JSON Schema defined in DPDK_context_schema.json.
*/

namespace DPDK {

/* This structure holds table attributes required for context JSON which are not
    part of P4Table */
struct TableAttributes {
    // Direction of the table, can be ["ingress","egress"]
    cstring direction;
    // Unique ID for the table
    unsigned tableHandle;
    /* Table type can one of "match", "selection" and "action
       Match table is a regular P4 table, selection table and action tables are compiler
       generated tables when psa_implementation is action_selector or action_profile */
    cstring tableType;
    bool is_add_on_miss;
    bool idle_timeout_with_auto_delete;
    bool isHidden;
    unsigned size;
    cstring controlName;
    cstring externalName;
    unsigned default_action_handle;
    /* Non selector table keys from original P4 program */
    std::vector<std::pair<cstring, cstring>> tableKeys;
};

/* This structure holds action attributes required for context JSON which are not
   part of P4Action */
struct actionAttributes {
    bool constant_default_action;
    bool is_compiler_added_action;
    bool allowed_as_hit_action;
    bool allowed_as_default_action;
    unsigned actionHandle;
    cstring externalName;
    IR::IndexedVector<IR::Parameter> *params;
};

struct externAttributes {
    cstring externalName;
    cstring externType;
    cstring counterType;
    unsigned table_id;
};

/* Program level information for context json */
struct TopLevelCtxt {
    cstring progName;
    cstring buildDate;
    cstring compileCommand;
    cstring compilerVersion;
    void initTopLevelCtxt(DpdkOptions &options) {
        buildDate = options.getBuildDate();
        compileCommand = options.getCompileCommand();
        progName = options.file;
        auto fileName = progName.findlast('/');
        // Handle the case when input file is in the current working directory.
        // fileName would be null in that case, hence progName should remain unchanged.
        if (fileName) progName = fileName;
        auto fileext = progName.find(".");
        progName = progName.replace(fileext, "");
        progName = progName.trim("/\t\n\r");
        compilerVersion = options.compilerVersion;
    }
};

/* Selection table attributes */
struct SelectionTable {
    unsigned max_n_groups;
    unsigned max_n_members_per_group;
    unsigned bound_to_action_data_table_handle;
    void setAttributes(const IR::P4Table *tbl,
                       const std::map<const cstring, struct TableAttributes> &tableAttrmap) {
        max_n_groups = 0;
        max_n_members_per_group = 0;
        auto n_groups = tbl->properties->getProperty("n_groups_max");
        if (n_groups) {
            auto n_groups_expr = n_groups->value->to<IR::ExpressionValue>()->expression;
            max_n_groups = n_groups_expr->to<IR::Constant>()->asInt();
        }
        auto n_members = tbl->properties->getProperty("n_members_per_group_max");
        if (n_members) {
            auto n_members_expr = n_members->value->to<IR::ExpressionValue>()->expression;
            max_n_members_per_group = n_members_expr->to<IR::Constant>()->asInt();
        }
        // Fetch associated member table handle
        cstring actionDataTableName = tbl->name.originalName;
        actionDataTableName = actionDataTableName.replace("_sel", "");
        auto actionTableAttr = ::get(tableAttrmap, actionDataTableName);
        bound_to_action_data_table_handle = actionTableAttr.tableHandle;
    }
};

// This pass generates context JSON into user specified file
class DpdkContextGenerator : public Inspector {
    P4::ReferenceMap *refmap;
    DpdkProgramStructure *structure;
    const p4configv1::P4Info &p4info;
    DpdkOptions &options;
    // All tables are collected into this vector
    IR::IndexedVector<IR::Declaration> tables;
    std::vector<const IR::Declaration_Instance *> externs;

    // Maps holding table, extern and action attributes needed for context JSON
    std::map<const cstring, struct TableAttributes> tableAttrmap;
    std::map<cstring, struct actionAttributes> actionAttrMap;
    std::map<cstring, struct externAttributes> externAttrMap;

    // Running unique ID for tables and actions
    std::map<cstring, size_t> context_handle_map;

 public:
    DpdkContextGenerator(P4::ReferenceMap *refmap, DpdkProgramStructure *structure,
                         const p4configv1::P4Info &p4info, DpdkOptions &options)
        : refmap(refmap), structure(structure), p4info(p4info), options(options) {}

    void serializeContextJson(std::ostream *destination);
    const Util::JsonObject *genContextJsonObject();
    void addMatchTables(Util::JsonArray *tablesJson);
    size_t getHandleId(cstring name);
    void collectHandleId();
    void addExternInfo(Util::JsonArray *externsJson);
    Util::JsonObject *initTableCommonJson(const cstring name, const struct TableAttributes &attr);
    void addKeyField(Util::JsonArray *keyJson, const cstring name, const cstring annon,
                     const IR::KeyElement *key, int position);
    Util::JsonArray *addActions(const IR::P4Table *table, const cstring ctrlName, bool isMatch);
    bool addRefTables(const cstring tbl_name, const IR::P4Table **memberTable,
                      Util::JsonObject *tableJson);
    void addImmediateField(Util::JsonArray *paramJson, const cstring name, int dest_start,
                           int dest_Width);
    void addActionParam(Util::JsonArray *paramJson, const cstring name, int bitWidth, int position,
                        int byte_array_index);
    Util::JsonObject *addMatchAttributes(const IR::P4Table *table, const cstring ctrlName);
    void setActionAttributes(const IR::P4Table *table);
    void setDefaultActionHandle(const IR::P4Table *table);
    void CollectTablesAndSetAttributes();
    cstring removePipePrefix(cstring);
};

}  // namespace DPDK

#endif /* BACKENDS_DPDK_DPDKCONTEXT_H_ */
