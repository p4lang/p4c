/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _BACKENDS_BMV2_PORTABLESWITCH_H_
#define _BACKENDS_BMV2_PORTABLESWITCH_H_

#include "ir/ir.h"
#include "lower.h"
#include "lib/gmputil.h"
#include "lib/json.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "helpers.h"

namespace P4 {

class PsaProgramStructure {
    BMV2::JsonObjects*   json;     // output json data structure

    ordered_map<cstring, IR::Type_Header> header_types;
    ordered_map<cstring, IR::Type_Struct> metadata_types;
    ordered_map<cstring, IR::Type_HeaderUnion> header_union_types;
    ordered_map<cstring, IR::Declaration_Variable> headers;
    ordered_map<cstring, IR::Declaration_Variable> header_stacks;
    ordered_map<cstring, IR::Declaration_Variable> header_unions;
    ordered_map<cstring, IR::Type_Error> errors;
    ordered_map<cstring, IR::Type_Enum> enums;
    ordered_map<cstring, IR::P4Parser> parsers;
    ordered_map<cstring, IR::P4ValueSet> parse_vsets;
    ordered_map<cstring, IR::P4Control> deparsers;
    ordered_map<cstring, IR::Declaration_Instance> meter_arrays;
    ordered_map<cstring, IR::Declaration_Instance> counter_arrays;
    ordered_map<cstring, IR::Declaration_Instance> register_arrays;
    ordered_map<cstring, IR::P4Action> actions;
    ordered_map<cstring, IR::P4Control> pipelines;
    // ordered_map<cstring, ???> calculations;  // P4-16 has no field list.
    ordered_map<cstring, IR::Declaration_Instance> checksums;
    // ordered_map<cstring, ??? > learn_lists;  // P4-16 has no field list;
    ordered_map<cstring, IR::Declaration_Instance> extern_instances;
    ordered_map<cstring, cstring> field_aliases;

public:
    PsaProgramStructure() {
        json = new BMV2::JsonObjects();
    }

    const IR::P4Program* create(const IR::P4Program* program);
    void createTypes();
    void createHeaders();
    void createParsers();
    void createExterns();
    void createActions();
    void createControls();
    void createDeparsers();
};

class InspectPsaProgram : public Inspector {
    PsaProgramStructure *structure;

public:
    explicit InspectPsaProgram(PsaProgramStructure *structure)
        : structure(structure) {
        CHECK_NULL(structure);
        setName("GeneratePsaProgram");
    }

    void postorder(const IR::P4Parser *p) override;
    void postorder(const IR::P4Control* c) override;
    void postorder(const IR::Type_Header* h) override;
    void postorder(const IR::Type_HeaderUnion* hu) override;
    void postorder(const IR::Declaration_Variable* var) override;
    void postorder(const IR::Declaration_Instance* di) override;
    void postorder(const IR::P4Action* act) override;
    void postorder(const IR::Type_Error* err) override;
};

class ConvertPsaProgramToJson : public Inspector {
    PsaProgramStructure *structure;

public:
    explicit ConvertPsaProgramToJson(PsaProgramStructure *structure)
        : structure(structure) {
        CHECK_NULL(structure);
        setName("GeneratePsaProgram");
    }

    bool preorder(const IR::P4Program *program) override {
        auto *rv = structure->create(program);
        return false;

    }
};

}  // namespace P4

#endif  /* _BACKENDS_BMV2_PORTABLESWITCH_H_ */
