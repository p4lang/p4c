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

#ifndef _MIDEND_ELIMINATESERENUMS_H_
#define _MIDEND_ELIMINATESERENUMS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Replaces serializable enum constants with their values.
 */
class DoEliminateSerEnums final : public Transform {
    const TypeMap* typeMap;
 public:
    explicit DoEliminateSerEnums(const TypeMap* typeMap): typeMap(typeMap)
    { setName("DoEliminateSerEnums"); }
    const IR::Node* preorder(IR::Type_SerEnum* type) override;
    const IR::Node* postorder(IR::Type_Name* type) override;
    const IR::Node* postorder(IR::Member* expression) override;
};

class EliminateSerEnums final : public PassManager {
 public:
    EliminateSerEnums(ReferenceMap* refMap, TypeMap* typeMap,
                      TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoEliminateSerEnums(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("EliminateSerEnums");
    }
};

}  // namespace P4

#endif /* _MIDEND_ELIMINATESERENUMS_H_ */
