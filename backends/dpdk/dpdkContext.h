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

#ifndef BACKENDS_DPDK_CONTEXT_H_
#define BACKENDS_DPDK_CONTEXT_H_

#include "dpdkProgramStructure.h"
#include "options.h"
#include "lib/nullstream.h"

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
    bool isHidden;
    cstring controlName;
    /* Non selector table keys from original P4 program */
    std::vector<cstring> tableKeys;
};

/* Collect tables and set table attributes for generating context JSON */
class CollectTablesForContextJson : public Inspector {
    DpdkProgramStructure *structure;
    // Vector of all tables in the program, includes compiler generated tables
    IR::IndexedVector<IR::Declaration> &tables;
    std::map<const cstring, struct TableAttributes> &tableAttrmap;
    static unsigned newTableHandle;

  public:
    CollectTablesForContextJson(DpdkProgramStructure *structure,
        IR::IndexedVector<IR::Declaration> &tables, std::map<const cstring,
        struct TableAttributes> &tableAttrmap)
        : structure(structure), tables(tables), tableAttrmap(tableAttrmap) {}

    bool preorder(const IR::P4Program *p) override;
    unsigned int getNewTableHandle();
    void setTableAttributes ();
};

/* This structure holds action attributes required for context JSON which are not
   part of P4Action */
struct actionAttributes {
    bool constant_default_action;
    bool is_compiler_added_action;
    bool allowed_as_hit_action;
    bool allowed_as_default_action;
    unsigned actionHandle;
    IR::IndexedVector<IR::Parameter> *params;
};

/* This pass outputs the context JSON into the file specified by user */
class WriteContextJson : public Inspector {
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
    DpdkProgramStructure *structure;
    DpdkOptions &options;
    IR::IndexedVector<IR::Declaration> &tables;
    std::map<const cstring, struct TableAttributes> &tableAttrmap;
    std::map <cstring, struct actionAttributes> actionAttrMap;
    static unsigned newActionHandle;
 public:
    WriteContextJson(P4::ReferenceMap *refmap, P4::TypeMap *typemap,
                     DpdkProgramStructure *structure, DpdkOptions &options,
                     IR::IndexedVector<IR::Declaration> &tables,
                     std::map<const cstring, struct TableAttributes> &tableAttrmap) :
                     refmap(refmap), typemap(typemap), structure(structure), options(options),
                     tables(tables), tableAttrmap(tableAttrmap) {}
    bool preorder(const IR::P4Program *p) override;
    unsigned int getNewActionHandle();
    void add_space(std::ostream &out, int size);
    void setActionAttributes (std::map <cstring, struct actionAttributes> &actionAttrMap,
                              const IR::P4Table *tbl);
    void printTableCtxtJson (const IR::P4Table *tbl, std::ostream &out);
    void printSelTableCtxtJson (const IR::P4Table *tbl, std::ostream &out);
    void printActionTableCtxtJson (const IR::P4Table *tbl, std::ostream &out);
};

class GenerateContextJson : public PassManager {
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
    DpdkProgramStructure *structure;
    DpdkOptions &options;
    IR::IndexedVector<IR::Declaration> tables;
    std::map<const cstring, struct TableAttributes> tableAttrmap;

public:
    GenerateContextJson(
        P4::ReferenceMap *refmap, P4::TypeMap *typemap,
        DpdkProgramStructure *structure,  DpdkOptions &options):
        refmap(refmap), typemap(typemap), structure(structure), options(options){
        addPasses( {
            new CollectTablesForContextJson(structure, tables, tableAttrmap),
            new WriteContextJson(refmap, typemap, structure, options, tables, tableAttrmap)
        });
    }
};
} // namespace DPDK
#endif

