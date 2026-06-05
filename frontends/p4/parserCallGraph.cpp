// Copyright 2016 VMware, Inc.
// SPDX-FileCopyrightText: 2016 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "parserCallGraph.h"

namespace P4 {

bool ComputeParserCG::preorder(const IR::PathExpression *expression) {
    auto state = findContext<IR::ParserState>();
    if (state != nullptr) {
        auto decl = getDeclaration(expression->path);
        if (decl != nullptr && decl->is<IR::ParserState>())
            transitions->calls(state, decl->to<IR::ParserState>());
    }
    return false;
}

void ComputeParserCG::postorder(const IR::SelectExpression *expression) {
    // transition (..) { ... } may imply a transition to
    // "reject" - if none of the cases matches.
    for (auto c : expression->selectCases) {
        if (c->keyset->is<IR::DefaultExpression>())
            // If we have a default case this will always match
            return;
    }
    auto state = findContext<IR::ParserState>();
    auto parser = findContext<IR::P4Parser>();
    CHECK_NULL(state);
    CHECK_NULL(parser);
    auto reject = parser->getDeclByName(IR::ParserState::reject);
    CHECK_NULL(reject);
    transitions->calls(state, reject->to<IR::ParserState>());
}

}  // namespace P4
