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

#ifndef FRONTENDS_P4_CHECKCOREMETHODS_H_
#define FRONTENDS_P4_CHECKCOREMETHODS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/// Check types for arguments of core.p4 methods
class DoCheckCoreMethods : public Inspector {
    ReferenceMap *refMap;
    TypeMap *typeMap;

    void checkEmitType(const IR::Expression *emit, const IR::Type *type) const;
    void checkCorelibMethods(const ExternMethod *em) const;

 public:
    DoCheckCoreMethods(ReferenceMap *refMap, TypeMap *typeMap) : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        setName("DoCheckCoreMethods");
    }

    void postorder(const IR::MethodCallExpression *expr) override;
};

class CheckCoreMethods : public PassManager {
 public:
    CheckCoreMethods(ReferenceMap *refMap, TypeMap *typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoCheckCoreMethods(refMap, typeMap));
        setName("CheckCoreMethods");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_CHECKCOREMETHODS_H_ */
