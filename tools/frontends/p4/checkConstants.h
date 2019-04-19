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

#ifndef _FRONTENDS_P4_CHECKCONSTANTS_H_
#define _FRONTENDS_P4_CHECKCONSTANTS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/// Makes sure that some methods that expect constant
/// arguments have constant arguments (e.g., push_front).
/// Checks that table sizes are constant integers.
class DoCheckConstants : public Inspector {
    ReferenceMap*  refMap;
    TypeMap*       typeMap;
 public:
    DoCheckConstants(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoCheckConstants");
    }

    void postorder(const IR::MethodCallExpression* expr) override;
    void postorder(const IR::KeyElement* expr) override;
    void postorder(const IR::P4Table* table) override;
};

class CheckConstants : public PassManager {
 public:
    CheckConstants(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoCheckConstants(refMap, typeMap));
        setName("CheckConstants");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CHECKCONSTANTS_H_ */
