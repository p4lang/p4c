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

#ifndef _MIDEND_FLATTENINTERFACESTRUCTS_H_
#define _MIDEND_FLATTENINTERFACESTRUCTS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

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
struct StructTypeReplacement : public IHasDbPrint {
    StructTypeReplacement(const P4::TypeMap* typeMap, const IR::Type_Struct* type);

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
    std::map<cstring, const IR::Type_Struct*> structFieldMap;
    // Holds a new flat type
    // struct M {
    //    bit _t_s_a0;
    //    bool _t_s_b1;
    //    bit<6> _t_y2;
    //    bit<3> _x3;
    // }
    const IR::Type* replacementType;
    virtual void dbprint(std::ostream& out) const {
        out << replacementType;
    }

    // Helper for constructor
    void flatten(const P4::TypeMap* typeMap,
                 cstring prefix,
                 const IR::Type* type,
                 IR::IndexedVector<IR::StructField> *fields);

    /// Returns a StructInitializerExpression suitable for
    /// initializing a struct for the fields that start with the
    /// given prefix.  For example, for prefix .t and root R this returns
    /// { .s = { .a = R._t_s_a0, .b = R._t_s_b1 }, .y = R._t_y2 }
    const IR::StructInitializerExpression* explode(
        const IR::Expression* root, cstring prefix);
};

/**
 * Maintains a map between an original struct type and its
 * replacement.
 */
struct NestedStructMap {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;

    ordered_map<const IR::Type*, StructTypeReplacement*> replacement;

    NestedStructMap(P4::ReferenceMap* refMap, P4::TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    void createReplacement(const IR::Type_Struct* type);
    StructTypeReplacement* getReplacement(const IR::Type* type) const
    { return ::get(replacement, type); }
    bool empty() const { return replacement.empty(); }
};

/**
Find the types to replace and insert them in the nested struct map.
This pass only looks at type arguments used in package instantiations.
 */
class FindTypesToReplace : public Inspector {
    NestedStructMap* map;
 public:
    explicit FindTypesToReplace(NestedStructMap* map): map(map) {
        setName("FindTypesToReplace");
        CHECK_NULL(map);
    }
    bool preorder(const IR::Declaration_Instance* inst) override;
};

/**
This pass transforms the type signatures of instantiated controls,
parsers, and packages.  It does not transform methods, functions or
actions.  It starts from package instantiations: every type argument
that is a nested structure is replaced with "simpler" flat type.

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
class ReplaceStructs : public Transform {
    NestedStructMap* replacementMap;
    std::map<const IR::Parameter*, StructTypeReplacement*> toReplace;

 public:
    explicit ReplaceStructs(NestedStructMap* sm): replacementMap(sm) {
        CHECK_NULL(sm);
        setName("ReplaceStructs");
    }

    const IR::Node* preorder(IR::P4Program* program) override;
    const IR::Node* postorder(IR::Member* expression) override;
    const IR::Node* preorder(IR::P4Parser* parser) override;
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::Type_Struct* type) override;
};

class FlattenInterfaceStructs final : public PassManager {
 public:
    FlattenInterfaceStructs(ReferenceMap* refMap, TypeMap* typeMap) {
        auto sm = new NestedStructMap(refMap, typeMap);
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new FindTypesToReplace(sm));
        passes.push_back(new ReplaceStructs(sm));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("FlattenInterfaceStructs");
    }
};


}  // namespace P4

#endif /* _MIDEND_FLATTENINTERFACESTRUCTS_H_ */
