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

#ifndef BACKENDS_COMMON_PORTABLEPROGRAMSTRUCTURE_H_
#define BACKENDS_COMMON_PORTABLEPROGRAMSTRUCTURE_H_

#include "backends/common/programStructure.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"

/// TODO: this is not really specific to BMV2, it should reside somewhere else.
namespace P4 {

class PortableProgramStructure : public P4::ProgramStructure {
 public:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    /// We place scalar user metadata fields (i.e., bit<>, bool)
    /// in the scalars map.
    ordered_map<cstring, const IR::Declaration_Variable *> scalars;
    unsigned scalars_width = 0;
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

    bool hasVisited(const IR::Type_StructLike *st) {
        if (auto h = st->to<IR::Type_Header>())
            return header_types.count(h->getName());
        else if (auto s = st->to<IR::Type_Struct>())
            return metadata_types.count(s->getName());
        else if (auto u = st->to<IR::Type_HeaderUnion>())
            return header_union_types.count(u->getName());
        return false;
    }
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

}  // namespace P4

#endif /* BACKENDS_COMMON_PORTABLEPROGRAMSTRUCTURE_H_ */
