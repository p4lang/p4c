/*
Copyright 2023 VMWare, Inc.

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

#ifndef P4_DEFAULTVALUES_H_
#define P4_DEFAULTVALUES_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Expand the 'default values' initializer ...
 */
class DoDefaultValues final : public Transform {
    TypeMap *typeMap;

    const IR::Expression *defaultValue(const IR::Expression *expression, const IR::Type *type);

 public:
    explicit DoDefaultValues(TypeMap *typeMap) : typeMap(typeMap) { CHECK_NULL(typeMap); }
    const IR::Node *postorder(IR::Dots *dots) override;
    const IR::Node *postorder(IR::StructExpression *expression) override;
    const IR::Node *postorder(IR::ListExpression *expression) override;
    const IR::Node *postorder(IR::HeaderStackExpression *expression) override;
};

class DefaultValues : public PassManager {
 public:
    DefaultValues(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (typeMap != nullptr) {
            if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap, true);
            passes.push_back(typeChecking);
        }
        passes.push_back(new DoDefaultValues(typeMap));
    }
};

}  // namespace P4

#endif /* P4_DEFAULTVALUES_H_ */
