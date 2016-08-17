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

#ifndef _MIDEND_SIMPLIFYEXPRESSIONS_H_
#define _MIDEND_SIMPLIFYEXPRESSIONS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

// convert expresions so that each expression contains at most one side-effect
// left-values are converted to contain no side-effects
// i.e. a[f(x)] = b
// is converted to
// tmp = f(x); a[tmp] = b
// m(n()) is converted to
// tmp = n()
// m(tmp)
class DoSimplifyExpressions : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;

    IR::IndexedVector<IR::Declaration> toInsert;
 public:
    // Currently this only works correctly only if initializers
    // cannot appear in local declarations - because we only have a global
    // toInsert vector.
    DoSimplifyExpressions(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoSimplifyExpressions");
    }

    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::ParserState* state) override;
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
    const IR::Node* postorder(IR::ReturnStatement* statement) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
    const IR::Node* postorder(IR::IfStatement* statement) override;
};

class SimplifyExpressions : public PassManager {
 public:
    SimplifyExpressions(ReferenceMap* refMap, TypeMap* typeMap, bool isv1) {
        passes.push_back(new TypeChecking(refMap, typeMap, isv1));
        passes.push_back(new DoSimplifyExpressions(refMap, typeMap));
        setName("SimplifyExpressions");
    }
};

}  // namespace P4

#endif /* _MIDEND_SIMPLIFYEXPRESSIONS_H_ */
