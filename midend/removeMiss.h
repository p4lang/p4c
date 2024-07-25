/*
Copyright 2019 VMware, Inc.

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

#ifndef MIDEND_REMOVEMISS_H_
#define MIDEND_REMOVEMISS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 *  This visitor converts table.apply().miss into !table.apply().hit.
 *  In an if statement it actually inverts the branches.
 */
class DoRemoveMiss : public Transform {
    ReferenceMap *refMap;
    TypeMap *typeMap;

 public:
    DoRemoveMiss(ReferenceMap *refMap, TypeMap *typeMap) : refMap(refMap), typeMap(typeMap) {
        visitDagOnce = false;
        CHECK_NULL(typeMap);
        setName("DoRemoveMiss");
    }
    const IR::Node *preorder(IR::Member *expression) override;
    const IR::Node *preorder(IR::IfStatement *statement) override;
};

class RemoveMiss : public PassManager {
 public:
    RemoveMiss(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoRemoveMiss(refMap, typeMap));
        setName("RemoveMiss");
    }
};

}  // namespace P4

#endif /* MIDEND_REMOVEMISS_H_ */
