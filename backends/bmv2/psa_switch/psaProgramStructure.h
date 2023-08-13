/*
Copyright 2013-present Barefoot Networks, Inc.
Copyright 2022 VMware Inc.

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

#ifndef BACKENDS_BMV2_PSA_SWITCH_PSAPROGRAMSTRUCTURE_H_
#define BACKENDS_BMV2_PSA_SWITCH_PSAPROGRAMSTRUCTURE_H_

#include <string.h>

#include <set>
#include <utility>
#include <vector>

#include "backends/bmv2/common/backend.h"
#include "backends/bmv2/common/programStructure.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/null.h"
#include "lib/ordered_map.h"

// TODO: this is not really specific to BMV2, it should reside somewhere else
namespace BMV2 {

class PsaProgramStructure : public ProgramStructure {
 protected:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

 public:
    // We place scalar user metadata fields (i.e., bit<>, bool)
    // in the scalars map.
    ordered_map<cstring, const IR::Declaration_Variable *> scalars;
    unsigned scalars_width = 0;
    unsigned error_width = 32;
    unsigned bool_width = 1;

    // architecture related information
    ordered_map<const IR::Node *, std::pair<gress_t, block_t>> block_type;

    ordered_map<cstring, const IR::Type_Header *> header_types;
    ordered_map<cstring, const IR::Type_Struct *> metadata_types;
    ordered_map<cstring, const IR::Type_HeaderUnion *> header_union_types;
    ordered_map<cstring, const IR::Declaration_Variable *> headers;
    ordered_map<cstring, const IR::Declaration_Variable *> metadata;
    ordered_map<cstring, const IR::Declaration_Variable *> header_stacks;
    ordered_map<cstring, const IR::Declaration_Variable *> header_unions;
    ordered_map<cstring, const IR::Type_Error *> errors;
    ordered_map<cstring, const IR::Type_Enum *> enums;
    ordered_map<cstring, const IR::P4Parser *> parsers;
    ordered_map<cstring, const IR::P4ValueSet *> parse_vsets;
    ordered_map<cstring, const IR::P4Control *> deparsers;
    ordered_map<cstring, const IR::P4Control *> pipelines;
    ordered_map<cstring, const IR::Declaration_Instance *> extern_instances;
    ordered_map<cstring, cstring> field_aliases;

    std::vector<const IR::ExternBlock *> globals;

 public:
    PsaProgramStructure(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }

    std::set<cstring> non_pipeline_controls;
    std::set<cstring> pipeline_controls;

    bool hasVisited(const IR::Type_StructLike *st) {
        if (auto h = st->to<IR::Type_Header>())
            return header_types.count(h->getName());
        else if (auto s = st->to<IR::Type_Struct>())
            return metadata_types.count(s->getName());
        else if (auto u = st->to<IR::Type_HeaderUnion>())
            return header_union_types.count(u->getName());
        return false;
    }

    /**
     * Checks if a string is of type PSA_CounterType_t returns true
     * if it is, false otherwise.
     */
    static bool isCounterMetadata(cstring ptName) { return !strcmp(ptName, "PSA_CounterType_t"); }

    /**
     * Checks if a string is a psa metadata returns true
     * if it is, false otherwise.
     */
    static bool isStandardMetadata(cstring ptName) {
        return (!strcmp(ptName, "psa_ingress_parser_input_metadata_t") ||
                !strcmp(ptName, "psa_egress_parser_input_metadata_t") ||
                !strcmp(ptName, "psa_ingress_input_metadata_t") ||
                !strcmp(ptName, "psa_ingress_output_metadata_t") ||
                !strcmp(ptName, "psa_egress_input_metadata_t") ||
                !strcmp(ptName, "psa_egress_deparser_input_metadata_t") ||
                !strcmp(ptName, "psa_egress_output_metadata_t"));
    }
};

class ParsePsaArchitecture : public Inspector {
    PsaProgramStructure *structure;

 public:
    explicit ParsePsaArchitecture(PsaProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }

    void modelError(const char *format, const IR::INode *node) {
        ::error(ErrorType::ERR_MODEL,
                (cstring(format) + "\nAre you using an up-to-date 'psa.p4'?").c_str(),
                node->getNode());
    }

    bool preorder(const IR::ToplevelBlock *block) override;
    bool preorder(const IR::PackageBlock *block) override;
    bool preorder(const IR::ExternBlock *block) override;

    profile_t init_apply(const IR::Node *root) override {
        structure->block_type.clear();
        structure->globals.clear();
        return Inspector::init_apply(root);
    }
};

class InspectPsaProgram : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    PsaProgramStructure *pinfo;

 public:
    InspectPsaProgram(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, PsaProgramStructure *pinfo)
        : refMap(refMap), typeMap(typeMap), pinfo(pinfo) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(pinfo);
        setName("InspectPsaProgram");
    }

    void postorder(const IR::P4Parser *p) override;
    void postorder(const IR::P4Control *c) override;
    void postorder(const IR::Declaration_Instance *di) override;

    bool isHeaders(const IR::Type_StructLike *st);
    void addTypesAndInstances(const IR::Type_StructLike *type, bool meta);
    void addHeaderType(const IR::Type_StructLike *st);
    void addHeaderInstance(const IR::Type_StructLike *st, cstring name);
    bool preorder(const IR::Declaration_Variable *dv) override;
    bool preorder(const IR::Parameter *parameter) override;
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_PSA_SWITCH_PSAPROGRAMSTRUCTURE_H_ */
