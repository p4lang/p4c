/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_CLONER_H_
#define FRONTENDS_P4_CLONER_H_

#include "ir/ir.h"

namespace P4 {

/// This transform converts identical PathExpression or Member nodes in a DAG
/// into distinct nodes.
class CloneExpressions : public Transform {
 public:
    CloneExpressions() {
        visitDagOnce = false;
        setName("CloneExpressions");
    }
    const IR::Node *postorder(IR::PathExpression *path) override {
        path->path = path->path->clone();
        return path;
    }

    // Clone expressions of the form Member(TypeNameExpression)
    const IR::Node *postorder(IR::Member *member) override {
        if (member->expr->is<IR::TypeNameExpression>()) {
            return new IR::Member(member->expr->clone(), member->member);
        }
        return member;
    }

    template <typename T>
    const T *clone(const IR::Node *node) {
        return node->apply(*this)->to<T>();
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_CLONER_H_ */
