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

#ifndef BACKENDS_COMMON_PSAPROGRAMSTRUCTURE_H_
#define BACKENDS_COMMON_PSAPROGRAMSTRUCTURE_H_

#include "portableProgramStructure.h"

namespace P4 {

enum block_t {
    PARSER,
    PIPELINE,
    DEPARSER,
};

class PsaProgramStructure : public PortableProgramStructure {
 public:
    /// Architecture related information.
    ordered_map<const IR::Node *, std::pair<gress_t, block_t>> block_type;

 public:
    PsaProgramStructure(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : PortableProgramStructure(refMap, typeMap) {}

    /// Checks if a string is of type PSA_CounterType_t returns true
    /// if it is, false otherwise.
    static bool isCounterMetadata(cstring ptName) { return !strcmp(ptName, "PSA_CounterType_t"); }

    /// Checks if a string is a psa metadata returns true
    /// if it is, false otherwise.
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

class ParsePsaArchitecture : public ParsePortableArchitecture {
    PsaProgramStructure *structure;

 public:
    explicit ParsePsaArchitecture(PsaProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }

    void modelError(const char *format, const IR::INode *node) {
        ::P4::error(ErrorType::ERR_MODEL,
                    (cstring(format) + "\nAre you using an up-to-date 'psa.p4'?").c_str(),
                    node->getNode());
    }

    bool preorder(const IR::PackageBlock *block) override;
    bool preorder(const IR::ExternBlock *block) override;

    profile_t init_apply(const IR::Node *root) override {
        structure->block_type.clear();
        structure->globals.clear();
        return Inspector::init_apply(root);
    }
};

class InspectPsaProgram : public InspectPortableProgram {
    PsaProgramStructure *pinfo;

 public:
    InspectPsaProgram(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, PsaProgramStructure *pinfo)
        : InspectPortableProgram(refMap, typeMap), pinfo(pinfo) {
        CHECK_NULL(pinfo);
        setName("InspectPsaProgram");
    }

    void postorder(const IR::P4Parser *p) override;
    void postorder(const IR::P4Control *c) override;
    void postorder(const IR::Declaration_Instance *di) override;

    void addTypesAndInstances(const IR::Type_StructLike *type, bool meta);
    void addHeaderType(const IR::Type_StructLike *st);
    void addHeaderInstance(const IR::Type_StructLike *st, cstring name);
    bool preorder(const IR::Declaration_Variable *dv) override;
    bool preorder(const IR::Parameter *parameter) override;
};

}  // namespace P4

#endif /* BACKENDS_COMMON_PSAPROGRAMSTRUCTURE_H_ */
