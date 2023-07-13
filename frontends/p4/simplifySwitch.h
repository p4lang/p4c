/*
Copyright 2021 VMware, Inc.

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

#ifndef FRONTENDS_P4_SIMPLIFYSWITCH_H_
#define FRONTENDS_P4_SIMPLIFYSWITCH_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/** @brief Simplify select and switch statements that have constant arguments.
 */
class DoSimplifySwitch : public Transform {
    TypeMap *typeMap;

    bool matches(const IR::Expression *left, const IR::Expression *right) const;

 public:
    explicit DoSimplifySwitch(TypeMap *typeMap) : typeMap(typeMap) {
        setName("DoSimplifySwitch");
        CHECK_NULL(typeMap);
    }

    const IR::Node *postorder(IR::SwitchStatement *stat) override;
};

class SimplifySwitch : public PassManager {
 public:
    SimplifySwitch(ReferenceMap *refMap, TypeMap *typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoSimplifySwitch(typeMap));
        setName("SimplifySwitch");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_SIMPLIFYSWITCH_H_ */
