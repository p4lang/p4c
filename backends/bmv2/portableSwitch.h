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
    ReferenceMap* refMap;
    TypeMap* typeMap;

 public:
    // We place scalar user metadata fields (i.e., bit<>, bool)
    // in the scalarsName metadata object, so we may need to rename
    // these fields.  This map holds the new names.
    ordered_map<cstring, const IR::StructField*> scalarMetadataFields;
    std::vector<const IR::StructField*> scalars;
    unsigned                            scalars_width = 0;
    unsigned                            error_width = 32;
    unsigned                            bool_width = 1;

    ordered_map<cstring, const IR::Type_Header*> header_types;
    ordered_map<cstring, const IR::Type_Struct*> metadata_types;
    ordered_map<cstring, const IR::Type_HeaderUnion*> header_union_types;
    ordered_map<cstring, const IR::Declaration_Variable*> headers;
    ordered_map<cstring, const IR::Declaration_Variable*> metadata;
    ordered_map<cstring, const IR::Declaration_Variable*> header_stacks;
    ordered_map<cstring, const IR::Declaration_Variable*> header_unions;
    ordered_map<cstring, const IR::Type_Error*> errors;
    ordered_map<cstring, const IR::Type_Enum*> enums;
    ordered_map<cstring, const IR::P4Parser*> parsers;
    ordered_map<cstring, const IR::P4ValueSet*> parse_vsets;
    ordered_map<cstring, const IR::P4Control*> deparsers;
    ordered_map<cstring, const IR::Declaration_Instance*> meter_arrays;
    ordered_map<cstring, const IR::Declaration_Instance*> counter_arrays;
    ordered_map<cstring, const IR::Declaration_Instance*> register_arrays;
    ordered_map<cstring, const IR::P4Action*> actions;
    ordered_map<cstring, const IR::P4Control*> pipelines;
    // ordered_map<cstring, ???> calculations;  // P4-16 has no field list.
    ordered_map<cstring, const IR::Declaration_Instance*> checksums;
    // ordered_map<cstring, ??? > learn_lists;  // P4-16 has no field list;
    ordered_map<cstring, const IR::Declaration_Instance*> extern_instances;
    ordered_map<cstring, cstring> field_aliases;

public:
    PsaProgramStructure(ReferenceMap* refMap, TypeMap* typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        json = new BMV2::JsonObjects();
    }

    const IR::P4Program* create(const IR::P4Program* program);
    void createStructLike(const IR::Type_StructLike* st);
    void createTypes();
    void createHeaders();
    void createParsers();
    void createExterns();
    void createActions();
    void createControls();
    void createDeparsers();
    BMV2::JsonObjects* getJson() { return json; }

    bool hasVisited(const IR::Type_StructLike* st) {
        if (auto h = st->to<IR::Type_Header>())
            return header_types.count(h->getName());
        else if (auto s = st->to<IR::Type_Struct>())
            return metadata_types.count(s->getName());
        else if (auto u = st->to<IR::Type_HeaderUnion>())
            return header_union_types.count(u->getName());
        return false;
    }
};

class ParsePsaArchitecture : public Inspector {
    PsaProgramStructure* structure;
 public:
    explicit ParsePsaArchitecture(PsaProgramStructure* structure) : structure(structure) { }

    bool preorder(const IR::ToplevelBlock* block) override;
    bool preorder(const IR::PackageBlock* block) override;
};

class InspectPsaProgram : public Inspector {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    PsaProgramStructure *pinfo;

 public:
    explicit InspectPsaProgram(ReferenceMap* refMap, TypeMap* typeMap, PsaProgramStructure *pinfo)
        : refMap(refMap), typeMap(typeMap), pinfo(pinfo) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(pinfo);
        setName("InspectPsaProgram");
    }

    void postorder(const IR::P4Parser *p) override;
    void postorder(const IR::P4Control* c) override;
    void postorder(const IR::Type_Header* h) override;
    void postorder(const IR::Type_HeaderUnion* hu) override;
    void postorder(const IR::Declaration_Variable* var) override;
    void postorder(const IR::Declaration_Instance* di) override;
    void postorder(const IR::P4Action* act) override;
    void postorder(const IR::Type_Error* err) override;

    // control
    bool preorder(const IR::P4Control *control) override;

    // parser
    bool preorder(const IR::P4Parser *p) override;

    // header
    std::set<cstring> visitedHeaders;
    bool isHeaders(const IR::Type_StructLike* st);
    void addTypesAndInstances(const IR::Type_StructLike* type, bool meta);
    void addHeaderType(const IR::Type_StructLike *st);
    void addHeaderInstance(const IR::Type_StructLike *st, cstring name);
    bool preorder(const IR::Parameter* parameter) override;
};

class ConvertToJson : public Inspector {
    PsaProgramStructure *structure;

public:
    explicit ConvertToJson(PsaProgramStructure *structure)
        : structure(structure) {
        CHECK_NULL(structure);
        setName("ConvertPsaProgramToJson");
    }

    bool preorder(const IR::P4Program *program) override {
        auto *rv = structure->create(program);
        return false;
    }
};

}  // namespace P4

#endif  /* _BACKENDS_BMV2_PORTABLESWITCH_H_ */
