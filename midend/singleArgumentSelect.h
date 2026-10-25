/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_SINGLEARGUMENTSELECT_H_
#define MIDEND_SINGLEARGUMENTSELECT_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
   Converts select(a, b, c) into select(a ++ b ++ c) &&& (maska ++ maskb ++ maskc).
   A similar transformation is done for the labels.
   @pre
   This should be run after SimplifySelectList and RemoveSelectBooleans.
   It assumes that all select arguments are scalar values of type Type_Bits.
*/
class DoSingleArgumentSelect : public Modifier {
    TypeMap *typeMap;
    const IR::Type *selectListType;

 public:
    explicit DoSingleArgumentSelect(TypeMap *typeMap) : typeMap(typeMap), selectListType(nullptr) {
        setName("DoSingleArgumentSelect");
    }

    /// A pair of expression representing an expression and a mask
    struct Pair {
        const IR::Expression *expr;
        const IR::Expression *mask;
        bool hasMask;

        Pair(const IR::Expression *source, const IR::Type *type);
    };

    // Validate that the expression contains only subexpressions
    // of supported types.
    void checkExpressionType(const IR::Expression *expression);

    bool preorder(IR::SelectCase *selCase) override;
    bool preorder(IR::SelectExpression *expression) override;
};

class SingleArgumentSelect : public PassManager {
 public:
    explicit SingleArgumentSelect(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoSingleArgumentSelect(typeMap));
        setName("SingleArgumentSelect");
    }
};

}  // namespace P4

#endif /* MIDEND_SINGLEARGUMENTSELECT_H_ */
