/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _FRONTENDS_P4_SIMPLIFY_H_
#define _FRONTENDS_P4_SIMPLIFY_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

// Determines whether an expression may have side-effects
class SideEffects : public Inspector {
 private:
    const ReferenceMap* refMap;
    const TypeMap*      typeMap;
    bool                hasSideEffects;
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
    SideEffects(const ReferenceMap* refMap, const TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), hasSideEffects(false) { setName("SideEffects"); }

 public:
    // Returns true if the expression may have side-effects.
    static bool check(const IR::Expression* expression,
                      const ReferenceMap* refMap,
                      const TypeMap* typeMap) {
        SideEffects se(refMap, typeMap);
        expression->apply(se);
        return se.hasSideEffects;
    }
};

class SimplifyControlFlow : public Transform {
    const ReferenceMap* refMap;
    const TypeMap*      typeMap;
 public:
    SimplifyControlFlow(const ReferenceMap* refMap, const TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("SimplifyControlFlow"); }
    const IR::Node* postorder(IR::BlockStatement* statement) override;
    const IR::Node* postorder(IR::IfStatement* statement) override;
    const IR::Node* postorder(IR::EmptyStatement* statement) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIMPLIFY_H_ */
