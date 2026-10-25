/*
 * Copyright 2020 VMware, Inc.
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_REASSOCIATION_H_
#define FRONTENDS_P4_REASSOCIATION_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

using namespace literals;

/** Implements a pass that reorders associative operations when beneficial.
 * For example, (a + c0) + c1 is rewritten as a + (c0 + c1) when cs are constants.
 */
class Reassociation final : public Transform {
 public:
    Reassociation() {
        visitDagOnce = true;
        setName("Reassociation");
    }
    using Transform::postorder;

    const IR::Node *reassociate(IR::Operation_Binary *root);

    const IR::Node *postorder(IR::Add *expr) override { return reassociate(expr); }
    const IR::Node *postorder(IR::Mul *expr) override { return reassociate(expr); }
    const IR::Node *postorder(IR::BOr *expr) override { return reassociate(expr); }
    const IR::Node *postorder(IR::BAnd *expr) override { return reassociate(expr); }
    const IR::Node *postorder(IR::BXor *expr) override { return reassociate(expr); }
    const IR::BlockStatement *preorder(IR::BlockStatement *bs) override {
        // FIXME: Do we need to check for expression, so we'd be able to fine tune, e.g.
        // @disable_optimization("reassociation")
        if (bs->hasAnnotation(IR::Annotation::disableOptimizationAnnotation)) prune();
        return bs;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_REASSOCIATION_H_ */
