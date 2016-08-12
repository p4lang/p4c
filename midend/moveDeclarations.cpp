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

void ResetHeaders::generateResets(const TypeMap* typeMap, const IR::Type* type,
                                  const IR::Expression* expr, IR::Vector<IR::StatOrDecl>* resets) {
    if (type->is<IR::Type_Struct>() || type->is<IR::Type_Union>()) {
        auto sl = type->to<IR::Type_StructLike>();
        for (auto f : *sl->fields) {
            auto ftype = typeMap->getType(f->type, true);
            auto member = new IR::Member(Util::SourceInfo(), expr, f->name);
            generateResets(typeMap, ftype, member, resets);
        }
    } else if (type->is<IR::Type_Header>()) {
        auto method = new IR::Member(expr->srcInfo, expr, IR::Type_Header::setInvalid);
        auto args = new IR::Vector<IR::Expression>();
        auto mc = new IR::MethodCallExpression(expr->srcInfo, method,
                                               new IR::Vector<IR::Type>(), args);
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        resets->push_back(stat);
    } else if (type->is<IR::Type_Stack>()) {
        auto tstack = type->to<IR::Type_Stack>();
        if (!tstack->sizeKnown()) {
            ::error("%1%: stack size is not a compile-time constant", tstack);
            return;
        }
        for (int i = 0; i < tstack->getSize(); i++) {
            auto index = new IR::Constant(i);
            auto elem = new IR::ArrayIndex(Util::SourceInfo(), expr, index);
            generateResets(typeMap, tstack->baseType, elem, resets);
        }
    }
}

const IR::Node* ResetHeaders::postorder(IR::Declaration_Variable* decl) {
    if (findContext<IR::ParserState>() == nullptr)
        return decl;
    if (decl->initializer != nullptr)
        return decl;
    auto resets = new IR::Vector<IR::StatOrDecl>();
    resets->push_back(decl);
    BUG_CHECK(getContext()->node->is<IR::Vector<IR::StatOrDecl>>(),
              "%1%: parent is not Vector<StatOrDecl>, but %2%",
              decl, getContext()->node);
    auto type = typeMap->getType(getOriginal(), true);
    auto path = new IR::PathExpression(decl->getName());
    generateResets(typeMap, type, path, resets);
    if (resets->size() == 1)
        return decl;
    return resets;
}

const IR::Node* MoveDeclarations::postorder(IR::P4Action* action)  {
    if (findContext<IR::P4Control>() == nullptr) {
        // Else let the parent control get these
        auto body = new IR::IndexedVector<IR::StatOrDecl>();
        auto m = getMoves();
        body->insert(body->end(), m->begin(), m->end());
        body->append(*action->body->components);
        action->body = new IR::BlockStatement(action->body->srcInfo, body);
        pop();
    }
    return action;
}

const IR::Node* MoveDeclarations::postorder(IR::P4Control* control)  {
    LOG1("Visiting " << control << " toMove " << toMove.size());
    auto decls = new IR::IndexedVector<IR::Declaration>();
    for (auto decl : *getMoves()) {
        LOG1("Moved " << decl);
        decls->push_back(decl);
    }
    decls->append(*control->controlLocals);
    control->controlLocals = decls;
    pop();
    return control;
}

const IR::Node* MoveDeclarations::postorder(IR::P4Parser* parser)  {
    auto newStateful = new IR::IndexedVector<IR::Declaration>();
    newStateful->append(*getMoves());
    newStateful->append(*parser->parserLocals);
    parser->parserLocals = newStateful;
    pop();
    return parser;
}

const IR::Node* MoveDeclarations::postorder(IR::Declaration_Variable* decl) {
    auto parent = getContext()->node;
    // We must keep the initializer here
    if (decl->initializer != nullptr &&
        !parent->is<IR::IndexedVector<IR::Declaration>>()) {
        // Don't split initializers from declarations that are at the "toplevel".
        // We cannot have assignment statements there.
        LOG1("Moving and splitting " << decl);
        auto move = new IR::Declaration_Variable(decl->srcInfo, decl->name,
                                                 decl->annotations, decl->type, nullptr);
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

const IR::Node* MoveDeclarations::postorder(IR::Declaration_Constant* decl) {
    if (findContext<IR::P4Control>() == nullptr &&
        findContext<IR::P4Action>() == nullptr &&
        findContext<IR::P4Parser>() == nullptr)
        // This is a global declaration
        return decl;
    addMove(decl);
    return nullptr;
}

const IR::Node* MoveInitializers::postorder(IR::Declaration_Variable* decl) {
    if (getContext()->parent == nullptr)
        return decl;
    auto parent = getContext()->parent->node;
    if (!parent->is<IR::P4Control>() &&
        !parent->is<IR::P4Parser>())
        // We are not in the local toplevel declarations
        return decl;
    if (decl->initializer == nullptr)
        return decl;

    auto varRef = new IR::PathExpression(decl->name);
    auto assign = new IR::AssignmentStatement(decl->srcInfo, varRef, decl->initializer);
    toMove->push_back(assign);
    decl->initializer = nullptr;
    return decl;
}

const IR::Node* MoveInitializers::postorder(IR::ParserState* state) {
    if (state->name != IR::ParserState::start ||
        toMove->empty())
        return state;
    toMove->append(*state->components);
    state->components = toMove;
    toMove = new IR::IndexedVector<IR::StatOrDecl>();
    return state;
}

const IR::Node* MoveInitializers::postorder(IR::P4Control* control) {
    if (toMove->empty())
        return control;
    toMove->append(*control->body->components);
    auto newBody = new IR::BlockStatement(Util::SourceInfo(), toMove);
    control->body = newBody;
    toMove = new IR::IndexedVector<IR::StatOrDecl>();
    return control;
}

}  // namespace P4
