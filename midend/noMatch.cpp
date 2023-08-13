/*
Copyright 2016 VMware, Inc.

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

#include "noMatch.h"

#include <ostream>
#include <string>
#include <vector>

#include "frontends/p4/coreLibrary.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/enumerator.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/log.h"

namespace P4 {

const IR::Node *DoHandleNoMatch::postorder(IR::SelectExpression *expression) {
    for (auto c : expression->selectCases) {
        if (c->keyset->is<IR::DefaultExpression>()) return expression;
    }

    if (noMatch == nullptr) {
        P4CoreLibrary &lib = P4CoreLibrary::instance();
        cstring name = nameGen->newName("noMatch");
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

const IR::Node *DoHandleNoMatch::postorder(IR::P4Parser *parser) {
    if (noMatch) parser->states.push_back(noMatch);
    noMatch = nullptr;
    return parser;
}

const IR::Node *DoHandleNoMatch::postorder(IR::P4Program *program) {
    if (!noMatch) return program;
    // Check if 'verify' exists.
    auto decls = program->getDeclsByName(IR::ParserState::verify);
    auto vec = decls->toVector();
    if (vec->empty()) {
        ::error(ErrorType::ERR_MODEL,
                "Declaration of function '%1%' not found; did you include core.p4?",
                IR::ParserState::verify);
        return program;
    }
    if (vec->size() > 1) {
        ::error(ErrorType::ERR_MODEL, "Multiple declarations of %1%: %2% %3%",
                IR::ParserState::verify, vec->at(0), vec->at(1));
    }
    return program;
}

}  // namespace P4
