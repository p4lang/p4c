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

#include "simplify.h"
#include "sideEffects.h"

namespace P4 {

const IR::Node* DoSimplifyControlFlow::postorder(IR::BlockStatement* statement) {
    LOG1("Visiting " << dbp(getOriginal()));
    if (statement->annotations->size() > 0)
        return statement;
    auto parent = getContext()->node;
    CHECK_NULL(parent);
    if (parent->is<IR::SwitchCase>() ||
        parent->is<IR::P4Control>() ||
        parent->is<IR::Function>() ||
        parent->is<IR::P4Action>()) {
        // Cannot remove these blocks
        return statement;
    }
    bool inBlock = findContext<IR::Statement>() != nullptr;
    bool inState = findContext<IR::ParserState>() != nullptr;
    if (!(inBlock || inState))
        return statement;

    if (parent->is<IR::BlockStatement>() || parent->is<IR::ParserState>()) {
        // if there are no local declarations we can remove this block
        bool hasDeclarations = false;
        for (auto c : statement->components)
            if (!c->is<IR::Statement>()) {
                hasDeclarations = true;
                break;
            }
        if (!hasDeclarations)
            return &statement->components;
    }

    if (statement->components.empty())
        return new IR::EmptyStatement(statement->srcInfo);
    if (statement->components.size() == 1) {
        auto first = statement->components.at(0);
        if (first->is<IR::Statement>())
            return first;
    }
    return statement;
}

const IR::Node* DoSimplifyControlFlow::postorder(IR::IfStatement* statement)  {
    LOG1("Visiting " << dbp(getOriginal()));
    if (SideEffects::check(statement->condition, refMap, typeMap))
        return statement;
    if (statement->ifTrue->is<IR::EmptyStatement>() &&
        (statement->ifFalse == nullptr || statement->ifFalse->is<IR::EmptyStatement>()))
        return new IR::EmptyStatement(statement->srcInfo);
    return statement;
}

const IR::Node* DoSimplifyControlFlow::postorder(IR::EmptyStatement* statement)  {
    LOG1("Visiting " << dbp(getOriginal()));
    auto parent = findContext<IR::Statement>();
    if (parent == nullptr ||  // in a ParserState or P4Action
        parent->is<IR::BlockStatement>())
        // remove it from blocks
        return nullptr;
    return statement;
}

const IR::Node* DoSimplifyControlFlow::postorder(IR::SwitchStatement* statement)  {
    LOG1("Visiting " << dbp(getOriginal()));
    if (statement->cases.empty()) {
        // The P4_16 spec prohibits expressions other than table application as
        // switch conditions.  The parser should have rejected programs for
        // which this is not the case.
        BUG_CHECK(statement->expression->is<IR::Member>(),
                  "%1%: expected a Member", statement->expression);
        auto expr = statement->expression->to<IR::Member>();
        BUG_CHECK(expr->expr->is<IR::MethodCallExpression>(),
                  "%1%: expected a table invocation", expr->expr);
        auto mce = expr->expr->to<IR::MethodCallExpression>();
        return new IR::MethodCallStatement(mce->srcInfo, mce);
    }
    auto last = statement->cases.back();
    if (last->statement == nullptr) {
        auto it = statement->cases.rbegin();
        for (; it != statement->cases.rend(); ++it) {
            if ((*it)->statement != nullptr)
                break;
            else
                ::warning(ErrorType::WARN_MISSING, "%1%: fallthrough with no statement", last); }
        statement->cases.erase(it.base(), statement->cases.end()); }
    return statement;
}

}  // namespace P4
