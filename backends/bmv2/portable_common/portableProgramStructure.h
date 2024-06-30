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

#ifndef BACKENDS_BMV2_PORTABLE_COMMON_PORTABLEPROGRAMSTRUCTURE_H_
#define BACKENDS_BMV2_PORTABLE_COMMON_PORTABLEPROGRAMSTRUCTURE_H_

#include "backends/bmv2/common/action.h"
#include "backends/bmv2/common/backend.h"
#include "backends/bmv2/common/control.h"
#include "backends/bmv2/common/extern.h"
#include "backends/bmv2/common/header.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/lower.h"
#include "backends/bmv2/common/programStructure.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/json.h"

/// TODO: this is not really specific to BMV2, it should reside somewhere else.
namespace BMV2 {

class PortableProgramStructure : public ProgramStructure {
 protected:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

 public:
    /// We place scalar user metadata fields (i.e., bit<>, bool)
    /// in the scalars map.
    ordered_map<cstring, const IR::Declaration_Variable *> scalars;
    unsigned scalars_width = 0;
    unsigned error_width = 32;
    unsigned bool_width = 1;

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
    PortableProgramStructure(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }

    std::set<cstring> non_pipeline_controls;
    std::set<cstring> pipeline_controls;

    void createStructLike(ConversionContext *ctxt, const IR::Type_StructLike *st);
    void createTypes(ConversionContext *ctxt);
    void createHeaders(ConversionContext *ctxt);
    void createScalars(ConversionContext *ctxt);
    void createExterns();
    void createActions(ConversionContext *ctxt);
    void createGlobals();
    cstring convertHashAlgorithm(cstring algo);

    bool hasVisited(const IR::Type_StructLike *st);
};

class ParsePortableArchitecture : public Inspector {
 public:
    bool preorder(const IR::ToplevelBlock *block) override;
};

class InspectPortableProgram : public Inspector {
 protected:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

 public:
    InspectPortableProgram(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }

    bool isHeaders(const IR::Type_StructLike *st);
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_PORTABLE_COMMON_PORTABLEPROGRAMSTRUCTURE_H_ */
