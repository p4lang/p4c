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
#include "frontends/p4/coreLibrary.h"

namespace P4 {

const IR::Node* DoHandleNoMatch::postorder(IR::SelectExpression* expression) {
    for (auto c : expression->selectCases) {
        if (c->keyset->is<IR::DefaultExpression>())
            return expression;
    }
    CHECK_NULL(noMatch);
    auto sc = new IR::SelectCase(
        new IR::DefaultExpression(), new IR::PathExpression(noMatch->getName()));
    expression->selectCases.push_back(sc);
    return expression;
}

const IR::Node* DoHandleNoMatch::preorder(IR::P4Parser* parser) {
    P4CoreLibrary& lib = P4CoreLibrary::instance;

    cstring name = nameGen->newName("noMatch");
    LOG2("Inserting " << name << " state");
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(new IR::BoolLiteral(false)));
    args->push_back(new IR::Argument(new IR::Member(
        new IR::TypeNameExpression(IR::Type_Error::error),
        lib.noMatch.Id())));
    auto verify = new IR::MethodCallExpression(
        new IR::PathExpression(IR::ID(IR::ParserState::verify)), args);
    noMatch = new IR::ParserState(IR::ID(name), { new IR::MethodCallStatement(verify) },
                                  new IR::PathExpression(IR::ID(IR::ParserState::reject)));
    parser->states.push_back(noMatch);
    return parser;
}

}  // namespace P4
