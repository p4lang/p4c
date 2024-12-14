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

#include "reassociation.h"

#include "ir/pattern.h"

namespace P4 {

void Reassociation::reassociate(IR::Operation_Binary *root) {
    LOG3("Trying to reassociate " << root);
    // Canonicalize constant to rhs
    if (root->left->is<IR::Constant>()) {
        std::swap(root->left, root->right);
        LOG3("Canonicalized constant to rhs: " << root);
    }

    // Match the following tree
    //       op
    //      /  \
    //     /    \
    //    op    c2
    //   / \
    //  /   \
    // e    c1
    //
    // (note that we're doing postorder visit and we already canonicalized
    // constants to rhs)
    // Rewrite to:
    //       op
    //      /  \
    //     /    \
    //    e     op
    //         /  \
    //        c1  c2
    const IR::Operation_Binary *lhs;
    const IR::Constant *c1, *c2;
    const IR::Expression *e;
    if (match(root, m_BinOp(m_CombineAnd(m_BinOp(lhs), m_BinOp(m_Expr(e), m_Constant(c1))),
                            m_Constant(c2))) &&
        lhs->getStringOp() == root->getStringOp()) {
        auto *newRight = root->clone();
        newRight->left = c1;
        newRight->right = c2;
        root->left = e;
        root->right = newRight;
        LOG3("Reassociated constants together: " << root);
    }
}

}  // namespace P4
