// Copyright 2016 VMware, Inc.
// SPDX-FileCopyrightText: 2016 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "noMatch.h"

#include "frontends/p4/coreLibrary.h"

namespace P4 {

const IR::Node *HandleNoMatch::postorder(IR::SelectExpression *expression) {
    for (auto c : expression->selectCases) {
        if (c->keyset->is<IR::DefaultExpression>()) return expression;
    }

    if (noMatch == nullptr) {
        P4CoreLibrary &lib = P4CoreLibrary::instance();
        cstring name = nameGen.newName("noMatch");
        LOG2("Inserting " << name << " state");
        auto args = new IR::Vector<IR::Argument>();
        args->push_back(new IR::Argument(new IR::BoolLiteral(false)));
        args->push_back(new IR::Argument(
            new IR::Member(new IR::TypeNameExpression(IR::Type_Error::error), lib.noMatch.Id())));
        auto verify = new IR::MethodCallExpression(
            new IR::PathExpression(IR::ID(IR::ParserState::verify)), args);
        noMatch = new IR::ParserState(IR::ID(name), {new IR::MethodCallStatement(verify)},
                                      new IR::PathExpression(IR::ID(IR::ParserState::reject)));
    }

    auto sc =
        new IR::SelectCase(new IR::DefaultExpression(), new IR::PathExpression(noMatch->getName()));
    expression->selectCases.push_back(sc);
    return expression;
}

const IR::Node *HandleNoMatch::postorder(IR::P4Parser *parser) {
    if (noMatch) parser->states.push_back(noMatch);
    noMatch = nullptr;
    return parser;
}

const IR::Node *HandleNoMatch::postorder(IR::P4Program *program) {
    if (!noMatch) return program;
    // Check if 'verify' exists.
    auto decls = program->getDeclsByName(IR::ParserState::verify);
    auto vec = decls->toVector();
    if (vec.empty()) {
        ::P4::error(ErrorType::ERR_MODEL,
                    "Declaration of function '%1%' not found; did you include core.p4?",
                    IR::ParserState::verify);
        return program;
    }
    if (vec.size() > 1) {
        ::P4::error(ErrorType::ERR_MODEL, "Multiple declarations of %1%: %2% %3%",
                    IR::ParserState::verify, vec[0], vec[1]);
    }
    return program;
}

}  // namespace P4
