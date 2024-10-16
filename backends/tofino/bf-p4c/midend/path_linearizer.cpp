/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "path_linearizer.h"

namespace BFN {

cstring LinearPath::to_cstring(cstring delimiter,
        bool skip_path_expression) {
    cstring ret = ""_cs;
    for (auto c : components) {
        if (auto pathexpr = c->to<IR::PathExpression>()) {
            if (skip_path_expression)
                continue;
            ret += pathexpr->path->name;
        } else if (auto member = c->to<IR::Member>()) {
            ret += member->member;
        } else if (auto href = c->to<IR::ConcreteHeaderRef>()) {
            ret += href->ref->name;
        }
        if (c != components.back())
            ret += delimiter;
    }
    return ret;
}

cstring LinearPath::to_cstring() {
    return to_cstring("."_cs, false);
}

Visitor::profile_t PathLinearizer::init_apply(const IR::Node* root) {
    BUG_CHECK(root->is<IR::PathExpression>() ||
              root->is<IR::Member>() ||
              root->is<IR::Slice>(),
              "Applying PathlikeExpressionAnalyzer to non-path-like "
              "expression: %1%", root);
    linearPath.emplace();
    return Inspector::init_apply(root);
}

void PathLinearizer::postorder(const IR::Path*) {
    // Just ignore Paths; they'll be represented by the PathExpressions that
    // contain them.
}

void PathLinearizer::postorder(const IR::PathExpression* path) {
    if (linearPath) linearPath->components.push_back(path);
}

void PathLinearizer::postorder(const IR::Member* member) {
    if (linearPath) linearPath->components.push_back(member);
}

void PathLinearizer::postorder(const IR::Slice* slice) {
    if (linearPath) linearPath->components.push_back(slice);
}

// When applied to P4-14 ConcreteHeaderRef, convert it to P4-16 PathExpression.
void PathLinearizer::postorder(const IR::ConcreteHeaderRef* href) {
    auto expr = new IR::PathExpression(href->srcInfo, href->type, new IR::Path(href->ref->name));
    if (linearPath) linearPath->components.push_back(expr);
}

bool PathLinearizer::preorder(const IR::HeaderOrMetadata*) {
    // Do not visit IR::Header underneath the HeaderRef.
    return false;
}

void PathLinearizer::postorder(const IR::ArrayIndex *array) {
    // TODO: currently dropped the index for array,
    // this is not correct, and should be fixed after 8.5
    if (array->left->is<IR::Member>())
        if (linearPath) linearPath->components.push_back(array->left->to<IR::Member>());
}

bool PathLinearizer::preorder(const IR::Constant *) {
    // Ignore constant; if they are used as ArrayIndex, it will
    // be represented in the ArrayIndex that contain them.
    return false;
}

void PathLinearizer::postorder(const IR::Node* node) {
    LOG2("Marking path-like expression invalid due to component: " << node);
    linearPath = std::nullopt;
}

void PathLinearizer::end_apply() {
    if (!linearPath) return;
    if (linearPath->components.empty()) {
        linearPath = std::nullopt;
        return;
    }
    if (!linearPath->components[0]->is<IR::PathExpression>() &&
        !linearPath->components[0]->is<IR::ConcreteHeaderRef>()) {
        LOG2("Marking path-like expression invalid due to first component: "
             << linearPath->components[0]);
        linearPath = std::nullopt;
        return;
    }
}

}  // namespace BFN
