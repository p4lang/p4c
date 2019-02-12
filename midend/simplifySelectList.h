/*
Copyright 2017 VMware, Inc.

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

#ifndef _MIDEND_SIMPLIFYSELECTLIST_H_
#define _MIDEND_SIMPLIFYSELECTLIST_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
Replace a reference to a structure with a list of all its fields
if the reference occurs in a select expression.  This should be run
after tuple elimination, so we only need to deal with structs.
*/
class SubstituteStructures : public Transform {
    TypeMap* typeMap;

    void explode(const IR::Expression* expression, const IR::Type* type,
                 IR::Vector<IR::Expression>* components);
 public:
    explicit SubstituteStructures(TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("SubstituteStructures"); }

    const IR::Node* postorder(IR::PathExpression* expression) override;
};

/**
Remove nested ListExpressions; this must be run after tuples have been eliminated
This is complicated because select labels have to be flattened too,
and a default expression in a select label can match a whole list.
For example, consider this example:
transition select(a, b, {c, d}) {
    default: accept;
    (0, 0, default): accept;
    (0, 0, {default, default}): accept;
}
This is converted to:
transition select(a, b, c, d) {
    default: accept;
    (0, 0, default, default): accept;
    (0, 0, default, default): accept;
}
*/
class UnnestSelectList : public Transform {
    // Represent the nesting of lists inside of a selectExpression.
    // E.g.: [__[__]_] for two nested lists.
    cstring nesting;

    void flatten(const IR::Expression* expression, IR::Vector<IR::Expression> *output);
    void flatten(const IR::Expression* expression, unsigned* nestingIndex,
                 IR::Vector<IR::Expression> *output);
 public:
    UnnestSelectList() { setName("UnnestSelectList"); }

    const IR::Node* preorder(IR::SelectExpression* expression) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { prune(); return control; }
};

class SimplifySelectList : public PassManager {
 public:
    SimplifySelectList(ReferenceMap* refMap, TypeMap* typeMap,
                       TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new SubstituteStructures(typeMap));
        passes.push_back(typeChecking);
        passes.push_back(new UnnestSelectList);
        setName("SimplifySelectList");
    }
};

}  // namespace P4

#endif /* _MIDEND_SIMPLIFYSELECTLIST_H_ */
