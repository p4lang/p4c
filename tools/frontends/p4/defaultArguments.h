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

#ifndef _FRONTENDS_P4_DEFAULTARGUMENTS_H_
#define _FRONTENDS_P4_DEFAULTARGUMENTS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

/**
 * Add arguments for parameters that have default values but no corresponding argument.
 * For example, given:
 * void f(in bit<32> a = 0);
 * convert
 * f();
 * to
 * f(a = 0);
 */
class DoDefaultArguments : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;

 public:
    DoDefaultArguments(ReferenceMap* refMap, TypeMap* typeMap): refMap(refMap), typeMap(typeMap)
    { setName("DoDefaultArguments"); CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::Declaration_Instance* inst) override;
    const IR::Node* postorder(IR::ConstructorCallExpression* ccc) override;
};

class DefaultArguments : public PassManager {
 public:
    DefaultArguments(ReferenceMap* refMap, TypeMap* typeMap) {
        setName("DefaultArguments");
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoDefaultArguments(refMap, typeMap));
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_DEFAULTARGUMENTS_H_ */
