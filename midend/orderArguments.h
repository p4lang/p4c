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

#ifndef _MIDEND_ORDERARGUMENTS_H_
#define _MIDEND_ORDERARGUMENTS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Order the arguments of a call in the order that the parameters appear.
 * This only works if all optional parameters are at the end.
 */
class DoOrderArguments : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    DoOrderArguments(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoOrderArguments"); }

    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::ConstructorCallExpression* expression) override;
    const IR::Node* postorder(IR::Declaration_Instance* instance) override;
};

class OrderArguments : public PassManager {
 public:
    OrderArguments(ReferenceMap* refMap, TypeMap* typeMap,
            TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoOrderArguments(refMap, typeMap));
        setName("OrderArguments");
    }
};

}  // namespace P4

#endif /* _MIDEND_ORDERARGUMENTS_H_ */
