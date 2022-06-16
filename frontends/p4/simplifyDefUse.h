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
#include "frontends/p4/cloner.h"

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
            // are processed as part of the control body
            return process(function);
        return function;
    }
    const IR::Node* postorder(IR::P4Parser* parser) override
    { return process(parser); }
    const IR::Node* postorder(IR::P4Control* control) override
    { return process(control); }
};

class SimplifyDefUse : public PassManager {
    class Cloner : public ClonePathExpressions {
     public:
        Cloner() { setName("Cloner"); }
        const IR::Node* postorder(IR::EmptyStatement* stat) override {
            // You cannot clone an empty statement, since
            // the visitor claims it's equal to the original one.
            // So we cheat and make an empty block.
            return new IR::BlockStatement(stat->srcInfo);
        }
    };


 public:
    SimplifyDefUse(ReferenceMap* refMap, TypeMap* typeMap,
             TypeChecking* typeChecking = nullptr) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);

        // SimplifyDefUse needs the expression tree *not* to be a DAG,
        // because it keeps state in hash-maps indexed with PathExpressions.
        // This is achieved by Cloner.
        passes.push_back(new Cloner());
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);

        auto repeated = new PassRepeated({
                typeChecking,
                new DoSimplifyDefUse(refMap, typeMap)
            });
        passes.push_back(repeated);
        setName("SimplifyDefUse");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIMPLIFYDEFUSE_H_ */
