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

#ifndef _FRONTENDS_P4_SIMPLIFYDEFUSE_H_
#define _FRONTENDS_P4_SIMPLIFYDEFUSE_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

class DoSimplifyDefUse : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;

    const IR::Node* process(const IR::Node* node);
 public:
    DoSimplifyDefUse(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoSimplifyDefUse");
    }

    const IR::Node* postorder(IR::Function* function) override {
        if (findContext<IR::Declaration_Instance>() == nullptr)
            // not an abstract function implementation: these
            // are processed as parat of the control body
            return process(function);
        return function;
    }
    const IR::Node* postorder(IR::P4Parser* parser) override
    { return process(parser); }
    const IR::Node* postorder(IR::P4Control* control) override
    { return process(control); }
};

class SimplifyDefUse : public PassManager {
 public:
    SimplifyDefUse(ReferenceMap* refMap, TypeMap* typeMap,
             TypeChecking* typeChecking = nullptr) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoSimplifyDefUse(refMap, typeMap));
        setName("SimplifyDefUse");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIMPLIFYDEFUSE_H_ */
