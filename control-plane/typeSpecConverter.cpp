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

#include <limits>
#include <map>
#include <string>

#include "p4/config/v1/p4types.pb.h"

#include "bytestrings.h"
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

bool hasTranslationAnnotation(const IR::Type* type,
                              TranslationAnnotation* payload) {
    auto ann = type->getAnnotation("p4runtime_translation");
    if (!ann) return false;

    // Syntax: @pruntime_translation(<uri>, <basic_type>).
    BUG_CHECK(ann->expr.size() == 2,
              "%1%: expected @p4runtime_translation annotation with 2 "
              "arguments, but found %2% arguments",
              type, ann->expr.size());
    const IR::Expression* first_arg = ann->expr[0];
    const IR::Expression* second_arg = ann->expr[1];

    auto uri = first_arg->to<IR::StringLiteral>();
    BUG_CHECK(uri != nullptr,
              "%1%: the first argument to @p4runtime_translation must be a "
              "string literal",
              type);
    payload->original_type_uri = std::string(uri->value);

    // See p4rtControllerType in p4parser.ypp for an explanation of how the
    // second argument is encoded.
    if (second_arg->to<IR::StringLiteral>() != nullptr) {
        payload->controller_type = ControllerType {
            .type = ControllerType::kString,
            .width = 0,
        };
        return true;
    } else if (second_arg->to<IR::Constant>() != nullptr) {
        payload->controller_type = ControllerType {
            .type = ControllerType::kBit,
            .width = second_arg->to<IR::Constant>()->asInt(),
        };
        return true;
    }
    BUG("%1%: expected second argument to @p4runtime_translation to parse as an"
        " IR::StringLiteral or IR::Constant, but got %2%",
        type, second_arg);
    return false;
}

cstring getTypeName(const IR::Type* type, TypeMap* typeMap) {
    CHECK_NULL(type);

    auto t = typeMap->getTypeType(type, true);
    if (auto newt = t->to<IR::Type_Newtype>()) {
        return newt->name;
    }
    return nullptr;
}

TypeSpecConverter::TypeSpecConverter(
    const P4::ReferenceMap* refMap, P4::TypeMap* typeMap, P4TypeInfo* p4RtTypeInfo)
    : refMap(refMap), typeMap(typeMap), p4RtTypeInfo(p4RtTypeInfo) {
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);
}

bool TypeSpecConverter::preorder(const IR::Type* type) {
    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected type %1%", type);
    map.emplace(type, new P4DataTypeSpec());
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_Bits* type) {
    auto typeSpec = new P4DataTypeSpec();
    auto bitTypeSpec = typeSpec->mutable_bitstring();
    auto bw = type->width_bits();
    if (type->isSigned) bitTypeSpec->mutable_int_()->set_bitwidth(bw);
    else
        bitTypeSpec->mutable_bit()->set_bitwidth(bw);
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

bool TypeSpecConverter::preorder(const IR::Type_Newtype* type) {
    if (p4RtTypeInfo) {
        auto name = std::string(type->controlPlaneName());
        auto types = p4RtTypeInfo->mutable_new_types();
        if (types->find(name) == types->end()) {
            auto newTypeSpec = new p4configv1::P4NewTypeSpec();

            // walk the chain of new types
            const IR::Type* underlyingType = type;
            while (underlyingType->is<IR::Type_Newtype>()) {
                underlyingType = typeMap->getTypeType(
                    underlyingType->to<IR::Type_Newtype>()->type, true);
            }

            TranslationAnnotation ann;
            bool isTranslatedType = hasTranslationAnnotation(type, &ann);
            if (isTranslatedType && !underlyingType->is<IR::Type_Bits>()) {
                ::error(ErrorType::ERR_INVALID,
                        "%1%: P4Runtime requires the underlying type for a user-defined type with "
                        "the @p4runtime_translation annotation to be bit<W>; it cannot be '%2%'",
                        type, underlyingType);
                // no need to return early here
            }
            visit(underlyingType);
            auto typeSpec = map.at(underlyingType);
            CHECK_NULL(typeSpec);

            if (isTranslatedType) {
                auto translatedType = newTypeSpec->mutable_translated_type();
                translatedType->set_uri(ann.original_type_uri);
                if (ann.controller_type.type == ControllerType::kString) {
                    translatedType->mutable_sdn_string();
                } else if (ann.controller_type.type == ControllerType::kBit) {
                    translatedType->set_sdn_bitwidth(ann.controller_type.width);
                } else {
                    BUG("Unexpected controller type: %1%",
                        ann.controller_type.type);
                }
            } else {
                newTypeSpec->mutable_original_type()->CopyFrom(*typeSpec);
            }
            (*types)[name] = *newTypeSpec;
       }
    }
    map.emplace(type, nullptr);
    return false;
}

bool TypeSpecConverter::preorder(const IR::Type_BaseList* type) {
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
    unsigned size = static_cast<unsigned>(sizeConstant->value);

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
            auto width = type->type->width_bits();
            bitTypeSpec->set_bitwidth(width);
            for (auto m : type->members) {
                auto member = enumTypeSpec->add_members();
                member->set_name(m->controlPlaneName());
                if (!m->value->is<IR::Constant>()) {
                    ::error(ErrorType::ERR_UNSUPPORTED,
                            "%1% unsupported SerEnum member value", m->value);
                    continue;
                }
                auto value = stringRepr(m->value->to<IR::Constant>(), width);
                if (!value) continue;  // error already logged by stringRepr
                member->set_value(*value);
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
    P4::TypeMap* typeMap,
    const IR::Type* type, P4TypeInfo* typeInfo) {
    TypeSpecConverter typeSpecConverter(refMap, typeMap, typeInfo);
    type->apply(typeSpecConverter);
    return typeSpecConverter.map.at(type);
}

}  // namespace ControlPlaneAPI

}  // namespace P4
