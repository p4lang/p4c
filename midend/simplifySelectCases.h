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

#ifndef _MIDEND_SIMPLIFYSELECTCASES_H_
#define _MIDEND_SIMPLIFYSELECTCASES_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

// Analyze reachability of select cases; handles corner cases
// (e.g., no cases).
// If requireConstants is true this pass requires that
// all select labels evaluate to constants.
class DoSimplifySelectCases : public Transform {
    const TypeMap* typeMap;
    bool requireConstants;

    void checkSimpleConstant(const IR::Expression* expr) const;

 public:
    DoSimplifySelectCases(const TypeMap* typeMap, bool requireConstants) :
            typeMap(typeMap), requireConstants(requireConstants)
    { setName("DoSimplifySelectCases"); }
    const IR::Node* preorder(IR::SelectExpression* expression) override;
};

class SimplifySelectCases : public PassManager {
 public:
    SimplifySelectCases(ReferenceMap* refMap, TypeMap* typeMap, bool requireConstants) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoSimplifySelectCases(typeMap, requireConstants));
        setName("SimplifySelectCases");
    }
};

}  // namespace P4

#endif /* _MIDEND_SIMPLIFYSELECTCASES_H_ */
