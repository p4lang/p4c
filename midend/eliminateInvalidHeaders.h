/*
 * Copyright 2022 VMware, Inc.
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_ELIMINATEINVALIDHEADERS_H_
#define MIDEND_ELIMINATEINVALIDHEADERS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Replaces invalid header expressions by variables
 * which are explicitly initialized to be invalid.
 * A similar operation is performed for invalid header unions.
 * This cannot be done for constant declarations, though.
 */
class DoEliminateInvalidHeaders final : public Transform {
    MinimalNameGenerator nameGen;
    IR::IndexedVector<IR::StatOrDecl> statements;
    std::vector<const IR::Declaration_Variable *> variables;

 public:
    DoEliminateInvalidHeaders() { setName("DoEliminateInvalidHeaders"); }

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = Transform::init_apply(node);
        node->apply(nameGen);

        return rv;
    }

    const IR::Node *postorder(IR::InvalidHeader *expression) override;
    const IR::Node *postorder(IR::InvalidHeaderUnion *expression) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::ParserState *parser) override;
    const IR::Node *postorder(IR::P4Action *action) override;
};

class EliminateInvalidHeaders final : public PassManager {
 public:
    EliminateInvalidHeaders(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoEliminateInvalidHeaders());
        setName("EliminateInvalidHeaders");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATEINVALIDHEADERS_H_ */
