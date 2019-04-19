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

#ifndef _MIDEND_COMPLEXCOMPARISON_H_
#define _MIDEND_COMPLEXCOMPARISON_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Implements a pass that removes comparisons on complex values
 * by converting them to comparisons on all their fields.
 */
class RemoveComplexComparisons : public Transform {
 protected:
    ReferenceMap* refMap;
    TypeMap* typeMap;

    /// Expands left == right into sub-field comparisons
    const IR::Expression* explode(
        Util::SourceInfo srcInfo,
        const IR::Type* leftType, const IR::Expression* left,
        const IR::Type* rightType, const IR::Expression* right);

 public:
    RemoveComplexComparisons(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("RemoveComplexComparisons"); }
    const IR::Node* postorder(IR::Operation_Binary* expression) override;
};

class SimplifyComparisons final : public PassManager {
 public:
    SimplifyComparisons(ReferenceMap* refMap, TypeMap* typeMap,
            TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new RemoveComplexComparisons(refMap, typeMap));
        setName("SimplifyComparisons");
    }
};


}  // namespace P4

#endif /* _MIDEND_COMPLEXCOMPARISON_H_ */
