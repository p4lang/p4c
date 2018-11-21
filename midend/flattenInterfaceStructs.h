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

#ifndef _MIDEND_FLATTENINTERFACESTRUCT_H_
#define _MIDEND_FLATTENINTERFACESTRUCT_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
   Describes how a type is replaced: the new type to
   replace it and how each field is renamed.
*/
struct Replacement : public IHasDbPrint {
    // The keys are strings of the form field.field.field.  If
    // something is mapped to the null string it does not really have
    // a replacement: this is true for fields whose types are structs.
    std::map<cstring, cstring> fieldNameRemap;
    const IR::Type* type;
    virtual void dbprint(std::ostream& out) const {
        out << type;
    }
};

/**
 * Maintains a map between an original struct type and its
 * replacement.
 */
struct NestedStructMap {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;

    ordered_map<const IR::Type*, Replacement*> replacement;

    NestedStructMap(P4::ReferenceMap* refMap, P4::TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    void createReplacement(const IR::Type_Struct* type);
    Replacement* getReplacement(const IR::Type* type) const
    { return ::get(replacement, type); }
    bool empty() const { return replacement.empty(); }
};

/**
Find the types to replace and insert them in the nested struct map.
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
that is a nested structure is replaced with "simpler" flatter type.

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
struct T { S s; }
struct Tflat {
   bit s_b;
}

control c(inout Tflat arg) {
    apply {
        ... arg.s_b; ...
    }
}

top<TFlat>(c()) main;

 */
class ReplaceStructs : public Transform {
    NestedStructMap* replacementMap;
    std::map<const IR::Parameter*, Replacement*> toReplace;

 public:
    ReplaceStructs(NestedStructMap* sm): replacementMap(sm) {
        CHECK_NULL(sm);
        setName("ReplaceStructs");
    }

    const IR::Node* preorder(IR::P4Program* program) override;
    const IR::Node* postorder(IR::Member* expression) override;
    const IR::Node* preorder(IR::P4Parser* parser) override;
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::Type_Name* type) override;
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

#endif /* _MIDEND_FLATTENINTERFACESTRUCT_H_ */
