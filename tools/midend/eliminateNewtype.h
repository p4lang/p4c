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

#ifndef _MIDEND_ELIMINATENEWTYPE_H_
#define _MIDEND_ELIMINATENEWTYPE_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Replaces newtype by the type it was defined to represent.
 * This can be done when all the information required by the
 * control-plane has been generated.  We actually just replace
 * the newtype by a typedef.
 */
class DoReplaceNewtype final : public Transform {
    const TypeMap* typeMap;
 public:
    explicit DoReplaceNewtype(const TypeMap* typeMap): typeMap(typeMap)
    { setName("DoReplaceNewtype"); }
    const IR::Node* postorder(IR::Type_Newtype* type) override {
        return new IR::Type_Typedef(type->srcInfo, type->name, type->type);
    }
    const IR::Node* postorder(IR::Cast* expression) override;
};

class EliminateNewtype final : public PassManager {
 public:
    EliminateNewtype(ReferenceMap* refMap, TypeMap* typeMap,
                     TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoReplaceNewtype(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("EliminateNewtype");
    }
};

}  // namespace P4

#endif /* _MIDEND_ELIMINATENEWTYPE_H_ */
