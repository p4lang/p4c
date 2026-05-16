/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_COMMON_LOWER_H_
#define BACKENDS_BMV2_COMMON_LOWER_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "midend/removeComplexExpressions.h"

namespace P4::BMV2 {

/// This pass rewrites expressions which are not supported natively on BMv2.
class LowerExpressions : public Transform {
    P4::TypeMap *typeMap;
    /// Maximum shift amount, defaults to 8 bits.
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
    explicit RemoveComplexExpressions(P4::TypeMap *typeMap,
                                      P4::RemoveComplexExpressionsPolicy *policy = nullptr)
        : P4::RemoveComplexExpressions(typeMap, policy) {}

    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_LOWER_H_ */
