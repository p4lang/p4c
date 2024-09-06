/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "moveDeclarations.h"

namespace P4 {

const IR::Node *MoveDeclarations::postorder(IR::P4Action *action) {
    if (findContext<IR::P4Control>() == nullptr) {
        // Else let the parent control get these
        auto body = new IR::IndexedVector<IR::StatOrDecl>();
        auto m = getMoves();
        body->insert(body->end(), m->begin(), m->end());
        body->append(action->body->components);
        action->body =
            new IR::BlockStatement(action->body->srcInfo, action->body->annotations, *body);
        pop();
    }
    return action;
}

const IR::Node *MoveDeclarations::postorder(IR::P4Control *control) {
    LOG1("Visiting " << control << " toMove " << toMove.size());
    auto decls = new IR::IndexedVector<IR::Declaration>();
    for (auto decl : *getMoves()) {
        LOG1("Moved " << decl);
        decls->push_back(decl);
    }
    decls->append(control->controlLocals);
    control->controlLocals = *decls;
    pop();
    return control;
}

const IR::Node *MoveDeclarations::postorder(IR::P4Parser *parser) {
    auto newStateful = new IR::IndexedVector<IR::Declaration>();
    newStateful->append(*getMoves());
    newStateful->append(parser->parserLocals);
    parser->parserLocals = *newStateful;
    pop();
    return parser;
}

const IR::Node *MoveDeclarations::postorder(IR::Function *function) {
    auto body = new IR::IndexedVector<IR::StatOrDecl>();
    auto m = getMoves();
    body->insert(body->end(), m->begin(), m->end());
    body->append(function->body->components);
    function->body =
        new IR::BlockStatement(function->body->srcInfo, function->body->annotations, *body);
    pop();
    return function;
}

const IR::Node *MoveDeclarations::postorder(IR::Declaration_Variable *decl) {
    auto parent = getContext()->node;
    // We must keep the initializer here
    if (decl->initializer != nullptr && !parent->is<IR::IndexedVector<IR::Declaration>>() &&
        !parent->is<IR::P4Parser>() && !parent->is<IR::P4Control>()) {
        // Don't split initializers from declarations that are at the "toplevel".
        // We cannot have assignment statements there.
        LOG1("Moving and splitting " << decl);
        auto move = new IR::Declaration_Variable(decl->srcInfo, decl->name, decl->annotations,
                                                 decl->type, nullptr);
        addMove(move);
        auto varRef = new IR::PathExpression(decl->name);
        auto keep = new IR::AssignmentStatement(decl->srcInfo, varRef, decl->initializer);
        return keep;
    } else {
        LOG1("Moving " << decl);
        addMove(decl);
        return nullptr;
    }
}

const IR::Node *MoveDeclarations::postorder(IR::Declaration_Constant *decl) {
    if (findContext<IR::P4Control>() == nullptr && findContext<IR::P4Action>() == nullptr &&
        findContext<IR::P4Parser>() == nullptr)
        // This is a global declaration
        return decl;
    addMove(decl);
    return nullptr;
}

////////////////////////////////////////////////////////////////////

const IR::Node *MoveInitializers::preorder(IR::P4Parser *parser) {
    // Determine if we have anything to do in this parser.
    // Check if we have top-level declarations with intializers.
    auto someInitializers = false;
    for (auto d : parser->parserLocals) {
        if (auto dv = d->to<IR::Declaration_Variable>()) {
            if (dv->initializer != nullptr) {
                someInitializers = true;
                break;
            }
        }
    }

    if (someInitializers) {
        // Check whether any of the parser's states loop back to the start state.
        // If this is the case, then the parser's decl initializers cannot be moved
        // to the start state. A new start state that is never looped back to must
        // be inserted at the head of the graph.
        for (const auto* state : parser->states) {
            const auto* selectExpr = state->selectExpression;
            if (selectExpr == nullptr) {
                // implicit reject transition
            } else if (selectExpr->is<IR::SelectExpression>()) {
                // select expression
                for (const auto* selectCase
                     : selectExpr->to<IR::SelectExpression>()->selectCases) {
                    if (selectCase->state->path->name == IR::ParserState::start) {
                        loopsBackToStart = true;
                        break;
                    }
                }
            } else if (selectExpr->is<IR::PathExpression>()) {
                // direct transition
                const auto* pathExpr = selectExpr->to<IR::PathExpression>();
                if (pathExpr->path->name == IR::ParserState::start) loopsBackToStart = true;
            }

            if (loopsBackToStart)
                break;
        }

        newStartName = nameGen.newName(IR::ParserState::start.string_view());
        oldStart = parser->states.getDeclaration(IR::ParserState::start)->to<IR::ParserState>();
        CHECK_NULL(oldStart);
    }
    return parser;
}

const IR::Node *MoveInitializers::preorder(IR::Declaration_Variable *decl) {
    if (getContext() == nullptr) return decl;
    auto parent = getContext()->node;
    if (!parent->is<IR::P4Control>() && !parent->is<IR::P4Parser>())
        // We are not in the local toplevel declarations
        return decl;
    if (decl->initializer == nullptr) return decl;

    auto varRef = new IR::PathExpression(decl->name);
    auto assign = new IR::AssignmentStatement(decl->srcInfo, varRef, decl->initializer);
    toMove->push_back(assign);
    decl->initializer = nullptr;
    return decl;
}

const IR::Node *MoveInitializers::postorder(IR::ParserState *state) {
    if (state->name == IR::ParserState::start && !toMove->empty()) {
        if (!loopsBackToStart) {
            // Just prepend all initializing assignments to the original start
            // state if there are no loops back to the start state.
            state->components.insert(state->components.begin(), (*toMove).begin(), (*toMove).end());
            toMove = new IR::IndexedVector<IR::StatOrDecl>();
        } else {
            // Otherwise, we need to insert all such assignments into a new start state
            // that can only run once, so that the assignments are only executed one time.
            state->name = newStartName;
        }
    }

    return state;
}

const IR::Node *MoveInitializers::postorder(IR::Path *path) {
    if (!oldStart || !loopsBackToStart || path->name != IR::ParserState::start)
        return path;

    // Only rename start state references if the parser contains initializing assignments
    // and loops back to the start state.
    auto decl = getDeclaration(getOriginal()->to<IR::Path>());
    if (!decl->is<IR::ParserState>()) return path;
    return new IR::Path(newStartName);
}

const IR::Node *MoveInitializers::postorder(IR::P4Parser *parser) {
    if (oldStart && loopsBackToStart) {
        // Only add a new state if the parser has initializers and contains one
        // or more states that loop back to the start state.
        auto newStart = new IR::ParserState(IR::ID(IR::ParserState::start), *toMove,
                                            new IR::PathExpression(newStartName));
        toMove = new IR::IndexedVector<IR::StatOrDecl>();
        parser->states.insert(parser->states.begin(), newStart);
    }

    oldStart = nullptr;
    loopsBackToStart = false;

    return parser;
}

const IR::Node *MoveInitializers::postorder(IR::P4Control *control) {
    if (toMove->empty()) return control;
    toMove->append(control->body->components);
    auto newBody =
        new IR::BlockStatement(control->body->srcInfo, control->body->annotations, *toMove);
    control->body = newBody;
    toMove = new IR::IndexedVector<IR::StatOrDecl>();
    return control;
}

Visitor::profile_t MoveInitializers::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);
    node->apply(nameGen);

    return rv;
}

}  // namespace P4
