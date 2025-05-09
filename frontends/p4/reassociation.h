/*
Copyright 2020 VMware, Inc.

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
