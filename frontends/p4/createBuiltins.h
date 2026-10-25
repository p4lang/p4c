/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_CREATEBUILTINS_H_
#define FRONTENDS_P4_CREATEBUILTINS_H_

#include "ir/ir.h"
#include "ir/visitor.h"

/*
 * Creates accept and reject states.
 * Adds parentheses to action invocations in tables:
    e.g., actions = { a; } becomes actions = { a(); }
 * Parser states without selects will transition to reject.
 */
namespace P4 {
class CreateBuiltins final : public Transform {
    bool addNoAction = false;
    /// toplevel declaration of NoAction.
    /// Checked only if it is referred.
    const IR::IDeclaration *globalNoAction;

    void checkGlobalAction();

 public:
    CreateBuiltins() {
        setName("CreateBuiltins");
        globalNoAction = nullptr;
    }
    const IR::Node *postorder(IR::ParserState *state) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::ActionListElement *element) override;
    const IR::Node *postorder(IR::ExpressionValue *property) override;
    const IR::Node *postorder(IR::Entry *property) override;
    const IR::Node *preorder(IR::P4Table *table) override;
    const IR::Node *postorder(IR::ActionList *actions) override;
    const IR::Node *postorder(IR::TableProperties *properties) override;
    const IR::Node *postorder(IR::Property *property) override;
    const IR::Node *preorder(IR::P4Program *program) override;
};
}  // namespace P4

#endif /* FRONTENDS_P4_CREATEBUILTINS_H_ */
