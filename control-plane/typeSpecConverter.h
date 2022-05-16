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

#ifndef CONTROL_PLANE_TYPESPECCONVERTER_H_
#define CONTROL_PLANE_TYPESPECCONVERTER_H_

#include <map>
#include <string>

#include "p4/config/v1/p4types.pb.h"

#include "ir/ir.h"
#include "ir/visitor.h"

namespace p4 {

class P4DataTypeSpec;
class P4TypeInfo;

}  // namespace p4

namespace P4 {

class TypeMap;
class ReferenceMap;

namespace ControlPlaneAPI {

/// Generates the appropriate p4.P4DataTypeSpec for a given IR::Type node.
class TypeSpecConverter : public Inspector {
 private:
    const P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    /// type_info field of the P4Info message: includes information about P4
    /// named types (struct, header, header union, enum, error).
    ::p4::config::v1::P4TypeInfo* p4RtTypeInfo;
    /// after translating an Expression to P4DataTypeSpec, save the result to
    /// 'map'.
    std::map<const IR::Type*, ::p4::config::v1::P4DataTypeSpec*> map;

    TypeSpecConverter(const P4::ReferenceMap* refMap,
                      P4::TypeMap* typeMap,
                      ::p4::config::v1::P4TypeInfo* p4RtTypeInfo);

    // fallback for unsupported types, should be unreachable
    bool preorder(const IR::Type* type) override;

    // anonymous types
    bool preorder(const IR::Type_Bits* type) override;
    bool preorder(const IR::Type_Varbits* type) override;
    bool preorder(const IR::Type_Boolean* type) override;
    bool preorder(const IR::Type_BaseList* type) override;
    bool preorder(const IR::Type_Stack* type) override;

    bool preorder(const IR::Type_Name* type) override;
    bool preorder(const IR::Type_Newtype* type) override;

    // these methods do not update the "map", but update p4RtTypeInfo if it is
    // not null.
    bool preorder(const IR::Type_Struct* type) override;
    bool preorder(const IR::Type_Header* type) override;
    bool preorder(const IR::Type_HeaderUnion* type) override;
    bool preorder(const IR::Type_Enum* type) override;
    bool preorder(const IR::Type_SerEnum* type) override;
    bool preorder(const IR::Type_Error* type) override;

 public:
    /// Generates the appropriate p4.P4DataTypeSpec message for @type. If
    /// @typeInfo is nullptr, then the relevant information is not generated for
    /// named types.
    static const ::p4::config::v1::P4DataTypeSpec* convert(
        const P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
        const IR::Type* type, ::p4::config::v1::P4TypeInfo* typeInfo);
};

/// See section "User-defined types" in P4RT specification.
struct ControllerType {
    enum Type { kBit, kString };
    Type type;  // Supported controller types are `string` and `bit<W>`.
    int width;  // 0 if type == kString.
};

/// Payload of @p4runtime_translation annotation.
struct TranslationAnnotation {
    std::string original_type_uri;
    ControllerType controller_type;
};

/// hasTranslationAnnotation returns true iff the type is annotated by a *valid*
/// p4runtime_translation annotation, in which case it populates the given
/// TranslationAnnotation with the values parsed from the annotation.
bool hasTranslationAnnotation(const IR::Type* type,
                              TranslationAnnotation* payload);

/// getTypeName returns a cstring for use as type_name for a Type_Newtype. It
/// returns nullptr if @type is not a Type_Newtype.
cstring getTypeName(const IR::Type* type, TypeMap* typeMap);

}  // namespace ControlPlaneAPI

}  // namespace P4

#endif  // CONTROL_PLANE_TYPESPECCONVERTER_H_
