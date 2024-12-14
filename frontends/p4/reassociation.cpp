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
    // n    c1
    //
    // (note that we're doing postorder visit and we already canonicalized
    // constants to rhs)
    // Rewrite to:
    //       op
    //      /  \
    //     /    \
    //    n     op
    //         /  \
    //        c1  c2
    // FIXME: Fix Pattern class to support matching in this scenario.

    const auto *c2 = root->right->to<IR::Constant>();
    if (!c2) return;
    auto *leftBin = root->left->to<IR::Operation_Binary>();
    if (!leftBin) return;
    if (leftBin->getStringOp() != root->getStringOp()) return;
    const auto *c1 = leftBin->right->to<IR::Constant>();
    if (!c1) return;

    auto *newRight = root->clone();
    newRight->left = c1;
    newRight->right = c2;
    root->left = leftBin->left;
    root->right = newRight;
    LOG3("Reassociate constants together: " << root);
}

}  // namespace P4
