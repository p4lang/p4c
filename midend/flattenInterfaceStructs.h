/*
Copyright 2018 VMware, Inc.

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

#ifndef MIDEND_FLATTENINTERFACESTRUCTS_H_
#define MIDEND_FLATTENINTERFACESTRUCTS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Policy to select which annotations of the nested struct to attach
 * to the struct fields after the nest struct is flattened.
 */
class AnnotationSelectionPolicy {
 public:
    virtual ~AnnotationSelectionPolicy() {}

    /**
     * Call for each nested struct to check if the annotation of the nested
     * struct should be kept on its fields.
     *
     * @return
     *  true if the annotation should be kept on the field.
     *  false if the annotation should be discarded.
     */
    virtual bool keep(const IR::Annotation *) { return true; }
};

/**
Describes how a nested struct type is replaced: the new type to
replace it and how each field is renamed.  For example, consider
the following:

struct S {
   bit a;
   bool b;
}

struct T {
   S s;
   bit<6> y;
}

struct M {
   T t;
   bit<3> x;
}
*/
template <typename T>
// T is the type of objects that will be replaced.  E.g., IR::Type_Struct
struct StructTypeReplacement : public IHasDbPrint {
    StructTypeReplacement(const P4::TypeMap *typeMap, const IR::Type_StructLike *type,
                          AnnotationSelectionPolicy *policy) {
        auto vec = new IR::IndexedVector<IR::StructField>();
        flatten(typeMap, "", type, type->annotations, vec, policy);
        if (type->is<IR::Type_Struct>()) {
            replacementType =
                new IR::Type_Struct(type->srcInfo, type->name, IR::Annotations::empty, *vec);
        } else if (type->is<IR::Type_Header>()) {
            replacementType =
                new IR::Type_Header(type->srcInfo, type->name, IR::Annotations::empty, *vec);
        } else {
            BUG("Unexpected type %1%", type);
        }
    }

    // Maps nested field names to final field names.
    // In our example this could be:
    // .t.s.a -> _t_s_a0;
    // .t.s.b -> _t_s_b1;
    // .t.y -> _t_y2;
    // .x -> _x3;
    std::map<cstring, cstring> fieldNameRemap;
    // Maps internal fields names to types.
    // .t -> T
    // .t.s -> S
    std::map<cstring, const IR::Type_StructLike *> structFieldMap;
    // Holds a new flat type
    // struct M {
    //    bit _t_s_a0;
    //    bool _t_s_b1;
    //    bit<6> _t_y2;
    //    bit<3> _x3;
    // }
    const IR::Type *replacementType;
    virtual void dbprint(std::ostream &out) const { out << replacementType; }

    // Helper for constructor
    void flatten(const P4::TypeMap *typeMap, cstring prefix, const IR::Type *type,
                 const IR::Annotations *annotations, IR::IndexedVector<IR::StructField> *fields,
                 AnnotationSelectionPolicy *policy) {
        // Drop name annotations
        IR::Annotations::Filter f = [](const IR::Annotation *a) {
            return a->name != IR::Annotation::nameAnnotation;
        };
        annotations = annotations->where(f);
        if (auto st = type->to<T>()) {
            std::function<bool(const IR::Annotation *)> selector =
                [&policy](const IR::Annotation *annot) {
                    if (!policy) return false;
                    return policy->keep(annot);
                };
            auto sannotations = st->annotations->where(selector);
            structFieldMap.emplace(prefix, st);
            for (auto f : st->fields) {
                auto na = new IR::Annotations();
                na->append(sannotations);
                na->append(annotations);
                na->append(f->annotations);
                auto ft = typeMap->getType(f, true);
                flatten(typeMap, prefix + "." + f->name, ft, na, fields, policy);
            }
            return;
        }
        cstring fieldName = prefix.replace(".", "_") + cstring::to_cstring(fieldNameRemap.size());
        fieldNameRemap.emplace(prefix, fieldName);
        fields->push_back(new IR::StructField(IR::ID(fieldName), annotations, type->getP4Type()));
        LOG3("Flatten: " << type << " | " << prefix);
    }

    /// Returns a StructExpression suitable for
    /// initializing a struct for the fields that start with the
    /// given prefix.  For example, for prefix .t and root R this returns
    /// { .s = { .a = R._t_s_a0, .b = R._t_s_b1 }, .y = R._t_y2 }
    const IR::StructExpression *explode(const IR::Expression *root, cstring prefix) {
        auto vec = new IR::IndexedVector<IR::NamedExpression>();
        auto fieldType = ::get(structFieldMap, prefix);
        BUG_CHECK(fieldType, "No field for %1%", prefix);
        for (auto f : fieldType->fields) {
            cstring fieldName = prefix + "." + f->name.name;
            auto newFieldname = ::get(fieldNameRemap, fieldName);
            const IR::Expression *expr;
            if (!newFieldname.isNullOrEmpty()) {
                expr = new IR::Member(root, newFieldname);
            } else {
                expr = explode(root, fieldName);
            }
            vec->push_back(new IR::NamedExpression(f->name, expr));
        }
        auto type = fieldType->getP4Type()->template to<IR::Type_Name>();
        return new IR::StructExpression(root->srcInfo, type, type, *vec);
    }
};

/**
 * Maintains a map between an original struct type and its
 * replacement.
 */
struct NestedStructMap {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    ordered_map<const IR::Type *, StructTypeReplacement<IR::Type_Struct> *> replacement;

    NestedStructMap(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }
    void createReplacement(const IR::Type_Struct *type);
    StructTypeReplacement<IR::Type_Struct> *getReplacement(const IR::Type *type) const {
        return ::get(replacement, type);
    }
    bool empty() const { return replacement.empty(); }
};

/**
Find the types to replace and insert them in the nested struct map.
This pass only looks at type arguments used in package instantiations.
 */
class FindTypesToReplace : public Inspector {
    NestedStructMap *map;

 public:
    explicit FindTypesToReplace(NestedStructMap *map) : map(map) {
        setName("FindTypesToReplace");
        CHECK_NULL(map);
    }
    bool preorder(const IR::Declaration_Instance *inst) override;
};

/**
This pass transforms the type signatures of instantiated controls,
parsers, and packages.  It does not transform methods, functions or
actions.  It starts from package instantiations: every type argument
that is a nested structure is replaced with "simpler" flat type.

This pass cannot handle methods or functions that return structs, or
that take an out argument of type struct.  If there are such
structures the pass will report an error.

Should be run after the NestedStructs pass.

struct S { bit b; }
struct T { S s; }

control c(inout T arg) {
    apply {
        ... arg.s.b ...
    }
}

control proto<V>(inout V arg);
package top<V>(proto<V> ctrl);

top<T>(c()) main;

This is transformed into:

struct S { bit b; }
struct T {
   bit _s_b0;
}

control c(inout T arg) {
    apply {
        ... arg._s_b0; ...
    }
}

top<TFlat>(c()) main;

 */
class ReplaceStructs : public Transform, P4WriteContext {
    NestedStructMap *replacementMap;
    std::map<const IR::Parameter *, StructTypeReplacement<IR::Type_Struct> *> toReplace;

 public:
    explicit ReplaceStructs(NestedStructMap *sm) : replacementMap(sm) {
        CHECK_NULL(sm);
        setName("ReplaceStructs");
    }

    const IR::Node *preorder(IR::P4Program *program) override;
    const IR::Node *postorder(IR::Member *expression) override;
    const IR::Node *preorder(IR::P4Parser *parser) override;
    const IR::Node *preorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::Type_Struct *type) override;
};

class FlattenInterfaceStructs final : public PassManager {
 public:
    FlattenInterfaceStructs(ReferenceMap *refMap, TypeMap *typeMap) {
        auto sm = new NestedStructMap(refMap, typeMap);
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new FindTypesToReplace(sm));
        passes.push_back(new ReplaceStructs(sm));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("FlattenInterfaceStructs");
    }
};

}  // namespace P4

#endif /* MIDEND_FLATTENINTERFACESTRUCTS_H_ */
