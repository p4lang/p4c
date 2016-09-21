/*
Copyright 2016 VMware, Inc.

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

#ifndef _MIDEND_SIDEEFFECTS_H_
#define _MIDEND_SIDEEFFECTS_H_

/* makes explicit side effect ordering */

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

// Determines whether an expression may have side-effects
// (but not any sub-expression)
class SideEffects : public Inspector {
 private:
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    bool          hasSideEffects;
    void postorder(const IR::MethodCallExpression* mce) override {
        if (refMap == nullptr || typeMap == nullptr) {
            // conservative
            hasSideEffects = true;
            return;
        }
        // TODO: add an annotation to indicate lack of side-effects
        auto mi = MethodInstance::resolve(mce, refMap, typeMap);
        if (!mi->is<BuiltInMethod>()) {
            hasSideEffects = true;
            return;
        }
        auto bim = mi->to<BuiltInMethod>();
        if (bim->name.name != IR::Type_Header::isValid)
            hasSideEffects = true;
    }
    void postorder(const IR::ConstructorCallExpression*) override {
        hasSideEffects = true;
    }
    // If you pass nullptr for these arguments the check will be more conservative
    SideEffects(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), hasSideEffects(false) { setName("SideEffects"); }

 public:
    // Returns true if the expression may have side-effects.
    static bool check(const IR::Expression* expression,
                      ReferenceMap* refMap,
                      TypeMap* typeMap) {
        SideEffects se(refMap, typeMap);
        expression->apply(se);
        return se.hasSideEffects;
    }
};

// convert expresions so that each expression contains at most one side-effect
// left-values are converted to contain no side-effects
// i.e. a[f(x)] = b
// is converted to
// tmp = f(x); a[tmp] = b
//
// m(n()) is converted to
// tmp = n()
// m(tmp)
//
// This is especially tricky for handling out and inout arguments.
// Consider bit f(inout T a, in T b); T g(inout T c);
// The expression a[g(w)].x = f(a[1].x, g(a[1].x)); is translated as
// T tmp1 = g(w);           // modifies w
// T tmp2 = a[1].x;         // save a[1].x
// T tmp3 = g(a[1].x);      // modifies a[1].x
// T tmp4 = f(tmp2, tmp3);  // modifies tmp2
// a[1].x = tmp2;           // copy tmp2 to a[1].x - out argument
// a[tmp1].x = tmp4;        // assign result of call of f to actual left value
// Function call proceeds as follows:
// - arguments are evaluated in order
// - inout and out arguments produce left-values - these are "saved"
// - function is called with temporaries for all arguments
// - out and inout temporaries are copied to the saved left-values in order
// An assignment statement
// e = e1;
// is treated as if it is a call to a function "set(out T v, in T v)"
class DoSimplifyExpressions : public Transform {
    ReferenceMap*        refMap;
    TypeMap*             typeMap;

    IR::IndexedVector<IR::Declaration> toInsert;
 public:
    // Currently this only works correctly only if initializers
    // cannot appear in local declarations.
    DoSimplifyExpressions(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoSimplifyExpressions");
    }

    const IR::Node* preorder(IR::P4Program* program) override {
        if (refMap->isV1()) prune();  // skip for P4 v1
        return program;
    }
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::ParserState* state) override;
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
    const IR::Node* postorder(IR::ReturnStatement* statement) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
    const IR::Node* postorder(IR::IfStatement* statement) override;
};

class SideEffectOrdering : public PassManager {
 public:
    SideEffectOrdering(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoSimplifyExpressions(refMap, typeMap));
        setName("SideEffectOrdering");
    }
};

}  // namespace P4

#endif /* _MIDEND_SIDEEFFECTS_H_ */
