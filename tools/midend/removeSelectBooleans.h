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

#ifndef _MIDEND_REMOVESELECTBOOLEANS_H_
#define _MIDEND_REMOVESELECTBOOLEANS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
Casts boolean expressions in a select or a select case keyset
to the type bit<1>.

We assume that this pass is run after SimplifySelectLists, so there
are only scalar types in the select expression.
*/
class DoRemoveSelectBooleans : public Transform {
    const P4::TypeMap* typeMap;

    const IR::Expression* addToplevelCasts(const IR::Expression* expression);
 public:
    explicit DoRemoveSelectBooleans(const P4::TypeMap* typeMap): typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoRemoveSelectBooleans");
    }

    const IR::Node* postorder(IR::SelectExpression* expression) override;
    const IR::Node* postorder(IR::SelectCase* selectCase) override;
};

class RemoveSelectBooleans : public PassManager {
 public:
    RemoveSelectBooleans(ReferenceMap* refMap, TypeMap* typeMap,
                         TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoRemoveSelectBooleans(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));  // some types have changed
        setName("RemoveSelectBooleans");
    }
};

}  // namespace P4

#endif /* _MIDEND_REMOVESELECTBOOLEANS_H_ */
