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

#include "lib/cstring.h"

namespace P4 {

const IR::Node *Reassociation::reassociate(IR::Operation_Binary *root) {
    auto right = root->right->to<IR::Constant>();
    if (!right) return root;
    auto leftBin = root->left->to<IR::Operation_Binary>();
    if (!leftBin) return root;
    if (leftBin->getStringOp() != root->getStringOp()) return root;
    if (!leftBin->right->is<IR::Constant>()) return root;
    auto c = dynamic_cast<IR::Operation_Binary *>(root->clone());
    c->left = leftBin->right;
    c->right = root->right;
    root->left = leftBin->left;
    root->right = c;
    return root;
}

}  // namespace P4
