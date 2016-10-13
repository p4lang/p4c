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

class DoSimplifyControlFlow : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
 public:
    DoSimplifyControlFlow(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoSimplifyControlFlow"); }
    const IR::Node* postorder(IR::BlockStatement* statement) override;
    const IR::Node* postorder(IR::IfStatement* statement) override;
    const IR::Node* postorder(IR::EmptyStatement* statement) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
};

class SimplifyControlFlow : public PassRepeated {
 public:
    SimplifyControlFlow(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoSimplifyControlFlow(refMap, typeMap));
        setName("SimplifyControlFlow");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIMPLIFY_H_ */
