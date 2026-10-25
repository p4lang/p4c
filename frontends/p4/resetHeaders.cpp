// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "resetHeaders.h"

namespace P4 {

void DoResetHeaders::generateResets(const TypeMap *typeMap, const IR::Type *type,
                                    const IR::Expression *expr,
                                    IR::Vector<IR::StatOrDecl> *resets) {
    if (type->is<IR::Type_Struct>() || type->is<IR::Type_HeaderUnion>()) {
        auto sl = type->to<IR::Type_StructLike>();
        for (auto f : sl->fields) {
            auto ftype = typeMap->getType(f, true);
            auto member = new IR::Member(expr, f->name);
            generateResets(typeMap, ftype, member, resets);
        }
    } else if (type->is<IR::Type_Header>()) {
        auto method = new IR::Member(expr->srcInfo, expr, IR::Type_Header::setInvalid);
        auto args = new IR::Vector<IR::Argument>();
        auto mc = new IR::MethodCallExpression(expr->srcInfo, method, args);
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        LOG3("Reset header " << expr);
        resets->push_back(stat);
    } else if (type->is<IR::Type_Array>()) {
        auto tstack = type->to<IR::Type_Array>();
        if (!tstack->sizeKnown()) {
            ::P4::error(ErrorType::ERR_UNSUPPORTED,
                        "%1%: stack size is not a compile-time constant", tstack);
            return;
        }
        for (unsigned i = 0; i < tstack->getSize(); i++) {
            auto index = new IR::Constant(i);
            auto elem = new IR::ArrayIndex(expr, index);
            generateResets(typeMap, tstack->elementType, elem, resets);
        }
    }
}

const IR::Node *DoResetHeaders::postorder(IR::Declaration_Variable *decl) {
    if (decl->initializer != nullptr) return decl;
    LOG3("DoResetHeaders context " << dbp(getContext()->node));
    auto parent = getContext()->node;
    // Don't reset index var in for..in statements.
    if (parent->is<IR::ForInStatement>()) return decl;
    auto type = typeMap->getType(getOriginal(), true);
    auto path = new IR::PathExpression(decl->getName());
    // For declarations in parsers and controls we have to insert the
    // reset in the start state or the body respectively.
    bool separate = parent->is<IR::P4Parser>() || parent->is<IR::P4Control>();
    if (!separate) {
        auto resets = new IR::Vector<IR::StatOrDecl>();
        resets->push_back(decl);
        generateResets(typeMap, type, path, resets);
        if (resets->size() == 1) return decl;
        return resets;
    } else {
        generateResets(typeMap, type, path, &insert);
        return decl;
    }
}

const IR::Node *DoResetHeaders::postorder(IR::P4Control *control) {
    insert.append(control->body->components);
    control->body =
        new IR::BlockStatement(control->body->srcInfo, control->body->annotations, insert);
    insert.clear();
    return control;
}

const IR::Node *DoResetHeaders::postorder(IR::ParserState *state) {
    if (state->name != IR::ParserState::start) return state;
    insert.append(state->components);
    state->components = insert;
    insert.clear();
    return state;
}

}  // namespace P4
