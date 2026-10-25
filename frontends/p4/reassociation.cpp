// Copyright 2020 VMware, Inc.
// SPDX-FileCopyrightText: 2020 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "reassociation.h"

namespace P4 {

const IR::Node *Reassociation::reassociate(IR::Operation_Binary *root) {
    const auto *right = root->right->to<IR::Constant>();
    if (!right) return root;
    auto leftBin = root->left->to<IR::Operation_Binary>();
    if (!leftBin) return root;
    if (leftBin->getStringOp() != root->getStringOp()) return root;
    if (!leftBin->right->is<IR::Constant>()) return root;
    auto *c = root->clone();
    c->left = leftBin->right;
    c->right = root->right;
    root->left = leftBin->left;
    root->right = c;
    return root;
}

}  // namespace P4
