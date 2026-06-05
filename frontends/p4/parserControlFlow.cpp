// Copyright 2016 VMware, Inc.
// SPDX-FileCopyrightText: 2016 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "parserControlFlow.h"

namespace P4 {

const IR::Node *DoRemoveParserControlFlow::postorder(IR::ParserState *state) {
    LOG1("Visiting " << dbp(state));
    // TODO: we keep annotations on the first state,
    // but this may be wrong for something like @atomic

    // Set of newly created states
    auto states = new IR::IndexedVector<IR::ParserState>();
    IR::ParserState *currentState = state;
    // components of the currentState
    IR::IndexedVector<IR::StatOrDecl> currentComponents;
    auto origComponents = state->components;
    auto origSelect = state->selectExpression;

    cstring joinName;  // non-empty if we split the state
    for (auto c : origComponents) {
        if (c->is<IR::IfStatement>()) {
            LOG1("Converting " << c << " into states");

            states->push_back(currentState);
            auto ifstat = c->to<IR::IfStatement>();
            joinName = nameGen.newName(state->name.name + "_join");

            // s_true
            cstring trueName = nameGen.newName(state->name.name + "_true");
            auto trueState = new IR::ParserState(trueName, {ifstat->ifTrue},
                                                 new IR::PathExpression(IR::ID(joinName, nullptr)));
            states->push_back(trueState);

            // s_false
            cstring falseName = joinName;
            if (ifstat->ifFalse != nullptr) {
                falseName = nameGen.newName(state->name.name + "_false");
                auto falseState =
                    new IR::ParserState(falseName, {ifstat->ifFalse},
                                        new IR::PathExpression(IR::ID(joinName, nullptr)));
                states->push_back(falseState);
            }

            // left-over
            auto trueCase = new IR::SelectCase(new IR::BoolLiteral(true),
                                               new IR::PathExpression(IR::ID(trueName, nullptr)));
            auto falseCase = new IR::SelectCase(new IR::BoolLiteral(false),
                                                new IR::PathExpression(IR::ID(falseName, nullptr)));
            currentState->selectExpression = new IR::SelectExpression(
                new IR::ListExpression({ifstat->condition}), {trueCase, falseCase});

            currentState->components = std::move(currentComponents);
            currentComponents.clear();
            currentState = new IR::ParserState(joinName, origSelect);  // may be overriten
        } else {
            currentComponents.push_back(c);
        }
    }
    currentState->components = std::move(currentComponents);

    if (states->empty()) return state;
    states->push_back(currentState);
    return states;
}

Visitor::profile_t DoRemoveParserControlFlow::init_apply(const IR::Node *node) {
    LOG1("DoRemoveControlFlow");
    auto rv = Transform::init_apply(node);

    node->apply(nameGen);

    return rv;
}

}  // namespace P4
