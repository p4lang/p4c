/*
Copyright 2020 VMware, Inc.

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

#include "eliminateInvalidHeaders.h"

#include <ostream>

#include "ir/id.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/log.h"
#include "lib/source_file.h"

namespace P4 {

const IR::Node *DoEliminateInvalidHeaders::postorder(IR::P4Control *control) {
    control->controlLocals.prepend(variables);
    auto vec = new IR::IndexedVector<IR::StatOrDecl>();
    vec->srcInfo = control->body->srcInfo;
    vec->append(statements);
    vec->append(control->body->components);
    control->body =
        new IR::BlockStatement(control->body->srcInfo, control->body->annotations, *vec);
    statements.clear();
    variables.clear();
    return control;
}

const IR::Node *DoEliminateInvalidHeaders::postorder(IR::ParserState *state) {
    state->components.prepend(statements);
    state->components.prepend(variables);
    variables.clear();
    statements.clear();
    return state;
}

const IR::Node *DoEliminateInvalidHeaders::postorder(IR::P4Action *action) {
    auto vec = new IR::IndexedVector<IR::StatOrDecl>();
    vec->append(variables);
    vec->append(statements);
    vec->append(action->body->components);
    action->body = new IR::BlockStatement(action->body->srcInfo, action->body->annotations, *vec);
    variables.clear();
    statements.clear();
    return action;
}

const IR::Node *DoEliminateInvalidHeaders::postorder(IR::InvalidHeader *expression) {
    if (!findContext<IR::BlockStatement>() && !findContext<IR::P4Action>() &&
        !findContext<IR::ParserState>()) {
        // We need some place to insert the setInvalid call.
        ::error("%1%: Cannot eliminate invalid header", expression);
        return expression;
    }
    cstring name = refMap->newName("ih");
    auto src = expression->srcInfo;
    auto decl = new IR::Declaration_Variable(name, expression->headerType->getP4Type());
    variables.push_back(decl);
    auto setInv = new IR::MethodCallStatement(
        src,
        new IR::MethodCallExpression(new IR::Member(
            src, new IR::PathExpression(src, new IR::Path(name)), IR::Type_Header::setInvalid)));
    statements.push_back(setInv);
    LOG2("Replacing " << expression << " with " << decl << " and " << setInv);
    return new IR::PathExpression(src, new IR::Path(name));
}

const IR::Node *DoEliminateInvalidHeaders::postorder(IR::InvalidHeaderUnion *expression) {
    if (!findContext<IR::BlockStatement>() && !findContext<IR::P4Action>() &&
        !findContext<IR::ParserState>()) {
        // We need some place to insert the setInvalid call.
        ::error("%1%: Cannot eliminate invalid header union", expression);
        return expression;
    }
    cstring name = refMap->newName("ih");
    auto src = expression->srcInfo;
    auto decl = new IR::Declaration_Variable(name, expression->headerUnionType->getP4Type());
    variables.push_back(decl);  // Uninitialized header unions are invalid
    LOG2("Replacing " << expression << " with " << decl);
    return new IR::PathExpression(src, new IR::Path(name));
}

}  // namespace P4
