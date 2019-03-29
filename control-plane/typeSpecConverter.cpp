/*
Copyright 2018-present Barefoot Networks, Inc.

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

#include <map>
#include <string>

#include "p4/config/v1/p4types.pb.h"

#include "flattenHeader.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/null.h"

#include "typeSpecConverter.h"

namespace p4configv1 = ::p4::config::v1;

using p4configv1::P4DataTypeSpec;
using p4configv1::P4TypeInfo;

namespace P4 {

namespace ControlPlaneAPI {

TypeSpecConverter::TypeSpecConverter(
    const P4::ReferenceMap* refMap, const P4::TypeMap* typeMap, P4TypeInfo* p4RtTypeInfo)
    : refMap(refMap), typeMap(typeMap), p4RtTypeInfo(p4RtTypeInfo) {
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);
}

bool TypeSpecConverter::preorder(const IR::Type* type) {
    ::error("Unexpected type %1%", type);
    map.emplace(type, new P4DataTypeSpec());
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Bits* type) {
    auto typeSpec = new P4DataTypeSpec();
    auto bitTypeSpec = typeSpec->mutable_bitstring();
    auto bw = type->width_bits();
    if (type->isSigned) bitTypeSpec->mutable_int_()->set_bitwidth(bw);
    else bitTypeSpec->mutable_bit()->set_bitwidth(bw);
    map.emplace(type, typeSpec);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Varbits* type) {
    auto typeSpec = new P4DataTypeSpec();
    auto bitTypeSpec = typeSpec->mutable_bitstring();
    bitTypeSpec->mutable_varbit()->set_max_bitwidth(type->size);
    map.emplace(type, typeSpec);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Boolean* type) {
    auto typeSpec = new P4DataTypeSpec();
    // enable "bool" field in P4DataTypeSpec's type_spec oneof
    (void)typeSpec->mutable_bool_();
    map.emplace(type, typeSpec);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Name* type) {
    auto typeSpec = new P4DataTypeSpec();
    auto decl = refMap->getDeclaration(type->path, true);
    auto name = decl->controlPlaneName();
    if (decl->is<IR::Type_Struct>()) {
        typeSpec->mutable_struct_()->set_name(name);
    } else if (decl->is<IR::Type_Header>()) {
        typeSpec->mutable_header()->set_name(name);
    } else if (decl->is<IR::Type_HeaderUnion>()) {
        typeSpec->mutable_header_union()->set_name(name);
    } else if (decl->is<IR::Type_Enum>()) {
        typeSpec->mutable_enum_()->set_name(name);
    } else if (decl->is<IR::Type_SerEnum>()) {
        typeSpec->mutable_serializable_enum()->set_name(name);
    } else if (decl->is<IR::Type_Error>()) {
        // enable "error" field in P4DataTypeSpec's type_spec oneof
        (void)typeSpec->mutable_error();
    } else if (decl->is<IR::Type_Typedef>()) {
        // Type_Name nodes for typedefs are only replaced in the midend, not the
        // frontend, but the P4Info generation happens after the frontend, so we
        // have to handle this case.
        auto typedefType = decl->to<IR::Type_Typedef>()->type;
        visit(typedefType);
        typeSpec = map.at(typedefType);
        CHECK_NULL(typeSpec);
        map.emplace(type, typeSpec);
        return false;
    } else if (decl->is<IR::Type_Newtype>()) {
        typeSpec->mutable_new_type()->set_name(name);
    } else {
        BUG("Unexpected named type %1%", type);
    }
    visit(decl->getNode());
    map.emplace(type, typeSpec);
    return false;
}

// This function is invoked if an architecure's model .p4 file has a Newtype.
// The function is not invoked for user-defined NewType.
// TODO(team): investigate and fix.
bool TypeSpecConverter::preorder(const IR::Type_Newtype* type) {
    if (p4RtTypeInfo) {
        bool orig_type = true;
        const IR::StringLiteral* uri;
        const IR::Constant* sdnB;
        auto ann = type->getAnnotation("p4runtime_translation");
        if (ann != nullptr) {
            orig_type = false;
            uri = ann->expr[0]->to<IR::StringLiteral>();
            if (!uri) {
                ::error("P4runtime annotation does not have uri: %1%",
                        type);
                return false;
            }
            sdnB = ann->expr[1]->to<IR::Constant>();
            if (!sdnB) {
                ::error("P4runtime annotation does not have sdn: %1%",
                        type);
                return false;
            }
            auto value = sdnB->value;
            auto bitsRequired = static_cast<size_t>(
                                mpz_sizeinbase(value.get_mpz_t(), 2));
            BUG_CHECK(bitsRequired <= 31,
                      "Cannot represent %1% on 31 bits, require %2%",
                      value, bitsRequired);
        }

        auto name = std::string(type->controlPlaneName());
        auto types = p4RtTypeInfo->mutable_new_types();
        if (types->find(name) == types->end()) {
            auto newTypeSpec = new p4configv1::P4NewTypeSpec();
            auto newType = type->type;
            auto n = newType->to<IR::Type_Name>();
            visit(n);
            auto typeSpec = map.at(n);
            if (orig_type) {
                auto dataType = newTypeSpec->mutable_original_type();
                if (typeSpec->has_bitstring())
                    dataType->mutable_bitstring()->CopyFrom(typeSpec->bitstring());
            } else {
                auto dataType = newTypeSpec->mutable_translated_type();
                dataType->set_uri(std::string(uri->value));
                dataType->set_sdn_bitwidth((uint32_t) sdnB->value.get_ui());
            }
            (*types)[name] = *newTypeSpec;
       }
    }
    map.emplace(type, nullptr);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Tuple* type) {
    auto typeSpec = new P4DataTypeSpec();
    auto tupleTypeSpec = typeSpec->mutable_tuple();
    for (auto cType : type->components) {
        visit(cType);
        auto cTypeSpec = map.at(cType);
        CHECK_NULL(cTypeSpec);
        auto member = tupleTypeSpec->add_members();
        member->CopyFrom(*cTypeSpec);
    }
    map.emplace(type, typeSpec);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Stack* type) {
    auto typeSpec = new P4DataTypeSpec();
    if (!type->elementType->is<IR::Type_Name>()) {
        BUG("Unexpected stack element type %1%", type->elementType);
    }

    auto decl = refMap->getDeclaration(type->elementType->to<IR::Type_Name>()->path, true);
    auto name = decl->controlPlaneName();

    auto sizeConstant = type->size->to<IR::Constant>();
    auto size = sizeConstant->value.get_ui();

    if (decl->is<IR::Type_Header>()) {
        auto headerStackTypeSpec = typeSpec->mutable_header_stack();
        headerStackTypeSpec->mutable_header()->set_name(name);
        headerStackTypeSpec->set_size(size);
    } else if (decl->is<IR::Type_HeaderUnion>()) {
        auto headerUnionStackTypeSpec = typeSpec->mutable_header_union_stack();
        headerUnionStackTypeSpec->mutable_header_union()->set_name(name);
        headerUnionStackTypeSpec->set_size(size);
    } else {
        BUG("Unexpected declaration %1%", decl);
    }
    visit(decl->getNode());
    map.emplace(type, typeSpec);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Struct* type) {
    if (p4RtTypeInfo) {
        auto name = std::string(type->controlPlaneName());
        auto structs = p4RtTypeInfo->mutable_structs();
        if (structs->find(name) == structs->end()) {
            auto structTypeSpec = new p4configv1::P4StructTypeSpec();
            for (auto f : type->fields) {
                auto fType = f->type;
                visit(fType);
                auto fTypeSpec = map.at(fType);
                CHECK_NULL(fTypeSpec);
                auto member = structTypeSpec->add_members();
                member->set_name(f->controlPlaneName());
                member->mutable_type_spec()->CopyFrom(*fTypeSpec);
            }
            (*structs)[name] = *structTypeSpec;
        }
    }
    map.emplace(type, nullptr);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Header* type) {
    auto flattenedHeaderType = FlattenHeader::flatten(typeMap, type);
    if (p4RtTypeInfo) {
        auto name = std::string(type->controlPlaneName());
        auto headers = p4RtTypeInfo->mutable_headers();
        if (headers->find(name) == headers->end()) {
            auto headerTypeSpec = new p4configv1::P4HeaderTypeSpec();
            for (auto f : flattenedHeaderType->fields) {
                auto fType = f->type;
                visit(fType);
                auto fTypeSpec = map.at(fType);
                CHECK_NULL(fTypeSpec);
                BUG_CHECK(fTypeSpec->has_bitstring(),
                          "Only bitstring fields expected in flattened header type %1%",
                          flattenedHeaderType);
                auto member = headerTypeSpec->add_members();
                member->set_name(f->controlPlaneName());
                member->mutable_type_spec()->CopyFrom(fTypeSpec->bitstring());
            }
            (*headers)[name] = *headerTypeSpec;
        }
    }
    map.emplace(type, nullptr);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_HeaderUnion* type) {
    if (p4RtTypeInfo) {
        auto name = std::string(type->controlPlaneName());
        auto headerUnions = p4RtTypeInfo->mutable_header_unions();
        if (headerUnions->find(name) == headerUnions->end()) {
            auto headerUnionTypeSpec = new p4configv1::P4HeaderUnionTypeSpec();
            for (auto f : type->fields) {
                auto fType = f->type;
                visit(fType);
                auto fTypeSpec = map.at(fType);
                CHECK_NULL(fTypeSpec);
                BUG_CHECK(fTypeSpec->has_header(),
                          "Only header fields expected in header union declaration %1%", type);
                auto member = headerUnionTypeSpec->add_members();
                member->set_name(f->controlPlaneName());
                member->mutable_header()->CopyFrom(fTypeSpec->header());
            }
            (*headerUnions)[name] = *headerUnionTypeSpec;
        }
    }
    map.emplace(type, nullptr);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Enum* type) {
    if (p4RtTypeInfo) {
        auto name = std::string(type->controlPlaneName());
        auto enums = p4RtTypeInfo->mutable_enums();
        if (enums->find(name) == enums->end()) {
            auto enumTypeSpec = new p4configv1::P4EnumTypeSpec();
            for (auto m : type->members) {
                auto member = enumTypeSpec->add_members();
                member->set_name(m->controlPlaneName());
            }
            (*enums)[name] = *enumTypeSpec;
        }
    }
    map.emplace(type, nullptr);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_SerEnum* type) {
    if (p4RtTypeInfo) {
        auto name = std::string(type->controlPlaneName());
        auto enums = p4RtTypeInfo->mutable_serializable_enums();
        if (enums->find(name) == enums->end()) {
            auto enumTypeSpec = new p4configv1::P4SerializableEnumTypeSpec();
            auto bitTypeSpec = enumTypeSpec->mutable_underlying_type();
            bitTypeSpec->set_bitwidth(type->type->width_bits());
            for (auto m : type->members) {
                auto member = enumTypeSpec->add_members();
                member->set_name(m->controlPlaneName());
                member->set_value(std::string(m->value->toString()));
            }
            (*enums)[name] = *enumTypeSpec;
        }
    }
    map.emplace(type, nullptr);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Error* type) {
    if (p4RtTypeInfo && !p4RtTypeInfo->has_error()) {
        auto errorTypeSpec = p4RtTypeInfo->mutable_error();
        for (auto m : type->members)
            errorTypeSpec->add_members(m->controlPlaneName());
    }
    map.emplace(type, nullptr);
    return false;
}

const P4DataTypeSpec* TypeSpecConverter::convert(
    const P4::ReferenceMap* refMap,
    const P4::TypeMap* typeMap,
    const IR::Type* type, P4TypeInfo* typeInfo) {
    TypeSpecConverter typeSpecConverter(refMap, typeMap, typeInfo);
    type->apply(typeSpecConverter);
    return typeSpecConverter.map.at(type);
}

}  // namespace ControlPlaneAPI

}  // namespace P4
