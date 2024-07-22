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

#ifndef FRONTENDS_P4_CHECKCONSTANTS_H_
#define FRONTENDS_P4_CHECKCONSTANTS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4C::P4 {

/// Makes sure that some methods that expect constant
/// arguments have constant arguments (e.g., push_front).
/// Checks that table sizes are constant integers.
class DoCheckConstants : public Inspector, public ResolutionContext {
    TypeMap *typeMap;

 public:
    explicit DoCheckConstants(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoCheckConstants");
    }

    void postorder(const IR::MethodCallExpression *expr) override;
    void postorder(const IR::KeyElement *expr) override;
    void postorder(const IR::P4Table *table) override;
};

class CheckConstants : public PassManager {
 public:
    explicit CheckConstants(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DoCheckConstants(typeMap));
        setName("CheckConstants");
    }
};

}  // namespace P4C::P4

#endif /* FRONTENDS_P4_CHECKCONSTANTS_H_ */
