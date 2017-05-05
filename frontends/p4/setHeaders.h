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

#ifndef _FRONTENDS_P4_SETHEADERS_H_
#define _FRONTENDS_P4_SETHEADERS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {
/**
Assigning a list expression to a header should also
set the header validity bit.  This pass should be run after
RemoveInitializers - it only looks at assignment statements.
*/
class DoSetHeaders : public Transform {
    const TypeMap* typeMap;

 public:
    static void generateSets(
        const TypeMap* typeMap, const IR::Type* type,
        const IR::Expression* expr, IR::Vector<IR::StatOrDecl>* resets);
    explicit DoSetHeaders(const TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("DoSetHeaders"); }
};

class SetHeaders : public PassManager {
 public:
    SetHeaders(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new P4::DoSetHeaders(typeMap));
        setName("SetHeaders");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SETHEADERS_H_ */
