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

#ifndef BACKENDS_BMV2_COMMON_LOWER_H_
#define BACKENDS_BMV2_COMMON_LOWER_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/null.h"
#include "midend/removeComplexExpressions.h"

namespace BMV2 {

/**
  This pass rewrites expressions which are not supported natively on BMv2.
*/
class LowerExpressions : public Transform {
    P4::TypeMap *typeMap;
    // Maximum shift amount, defaults to 8 bits
    int maxShiftWidth;

    const IR::Expression *shift(const IR::Operation_Binary *expression) const;

 public:
    explicit LowerExpressions(P4::TypeMap *typeMap, int maxShiftWidth = 8)
        : typeMap(typeMap), maxShiftWidth(maxShiftWidth) {
        CHECK_NULL(typeMap);
        setName("LowerExpressions");
    }

    const IR::Node *postorder(IR::Expression *expression) override;
    const IR::Node *postorder(IR::Shl *expression) override { return shift(expression); }
    const IR::Node *postorder(IR::Shr *expression) override { return shift(expression); }
    const IR::Node *postorder(IR::Cast *expression) override;
    const IR::Node *postorder(IR::Neg *expression) override;
    const IR::Node *postorder(IR::Slice *expression) override;
    const IR::Node *postorder(IR::Concat *expression) override;
    const IR::Node *preorder(IR::P4Table *table) override {
        prune();
        return table;
    }  // don't simplify expressions in table
};

class RemoveComplexExpressions : public P4::RemoveComplexExpressions {
 public:
    RemoveComplexExpressions(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                             P4::RemoveComplexExpressionsPolicy *policy = nullptr)
        : P4::RemoveComplexExpressions(refMap, typeMap, policy) {}

    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_LOWER_H_ */
