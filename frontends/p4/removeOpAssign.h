/*
 * Copyright 2025 Nvidia, Inc.
 * SPDX-FileCopyrightText: 2025 Nvidia, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_REMOVEOPASSIGN_H_
#define FRONTENDS_P4_REMOVEOPASSIGN_H_

#include "cloner.h"
#include "ir/ir.h"
#include "sideEffects.h"

namespace P4 {

class RemoveOpAssign : public Transform {
    template <class T>
    const IR::Node *doit(T *as) {
        prune();
        BUG_CHECK(!SideEffects::check(as->left, this), "side effects in LHS of %s", as);
        return new IR::AssignmentStatement(as->srcInfo, as->left->apply(CloneExpressions()),
                                           new typename T::BinOp(as->left, as->right));
    }

#define PREORDER(OP) \
    const IR::Node *preorder(IR::OP *as) override { return doit(as); }
    PREORDER(MulAssign)
    PREORDER(DivAssign)
    PREORDER(ModAssign)
    PREORDER(AddAssign)
    PREORDER(SubAssign)
    PREORDER(AddSatAssign)
    PREORDER(SubSatAssign)
    PREORDER(ShlAssign)
    PREORDER(ShrAssign)
    PREORDER(BAndAssign)
    PREORDER(BOrAssign)
    PREORDER(BXorAssign)
#undef PREORDER

    const IR::Node *preorder(IR::AssignmentStatement *s) override {
        prune();
        return s;
    }
    const IR::Node *preorder(IR::Expression *e) override {
        prune();
        return e;
    }
    const IR::Node *preorder(IR::Annotation *a) override {
        prune();
        return a;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_REMOVEOPASSIGN_H_ */
