/*
Copyright 2024 Marvell Technology, Inc.

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

#ifndef BACKENDS_BMV2_PNA_NIC_PNAPROGRAMSTRUCTURE_H_
#define BACKENDS_BMV2_PNA_NIC_PNAPROGRAMSTRUCTURE_H_

#include "backends/common/portableProgramStructure.h"

/// TODO: this is not really specific to BMV2, it should reside somewhere else
namespace P4::BMV2 {

enum pna_block_t {
    MAIN_PARSER,
    MAIN_CONTROL,
    MAIN_DEPARSER,
};

class PnaProgramStructure : public P4::PortableProgramStructure {
 public:
    /// Architecture related information
    ordered_map<const IR::Node *, pna_block_t> block_type;

 public:
    PnaProgramStructure(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : PortableProgramStructure(refMap, typeMap) {}

    /// Checks if a string is of type PNA_CounterType_t returns true
    /// if it is, false otherwise.
    static bool isCounterMetadata(cstring ptName) { return !strcmp(ptName, "PNA_CounterType_t"); }

    /// Checks if a string is a pna metadata returns true
    /// if it is, false otherwise.
    static bool isStandardMetadata(cstring ptName) {
        return (!strcmp(ptName, "pna_main_parser_input_metadata_t") ||
                !strcmp(ptName, "pna_main_input_metadata_t") ||
                !strcmp(ptName, "pna_main_output_metadata_t"));
    }
};

class ParsePnaArchitecture : public P4::ParsePortableArchitecture {
    PnaProgramStructure *structure;

 public:
    explicit ParsePnaArchitecture(PnaProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }

    void modelError(const char *format, const IR::INode *node) {
        ::P4::error(ErrorType::ERR_MODEL,
                    (cstring(format) + "\nAre you using an up-to-date 'pna.p4'?").c_str(),
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

class InspectPnaProgram : public P4::InspectPortableProgram {
    PnaProgramStructure *pinfo;

 public:
    InspectPnaProgram(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, PnaProgramStructure *pinfo)
        : InspectPortableProgram(refMap, typeMap), pinfo(pinfo) {
        CHECK_NULL(pinfo);
        setName("InspectPnaProgram");
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

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_PNA_NIC_PNAPROGRAMSTRUCTURE_H_ */
