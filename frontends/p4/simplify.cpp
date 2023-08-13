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

#include <ostream>
#include <string>
#include <vector>

#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error_catalog.h"
#include "lib/log.h"
#include "sideEffects.h"
#include "tableApply.h"

namespace P4 {

const IR::Node *DoSimplifyControlFlow::postorder(IR::BlockStatement *statement) {
    LOG3("Visiting " << dbp(getOriginal()));
    if (statement->annotations->size() > 0) return statement;
    auto parent = getContext()->node;
    CHECK_NULL(parent);
    if (parent->is<IR::SwitchCase>() || parent->is<IR::P4Control>() || parent->is<IR::Function>() ||
        parent->is<IR::P4Action>()) {
        // Cannot remove these blocks
        return statement;
    }
    bool inBlock = findContext<IR::Statement>() != nullptr;
    bool inState = findContext<IR::ParserState>() != nullptr;
    if (!(inBlock || inState)) return statement;

    if (parent->is<IR::BlockStatement>() || parent->is<IR::ParserState>()) {
        // if there are no local declarations we can remove this block
        bool hasDeclarations = false;
        for (auto c : statement->components)
            if (!c->is<IR::Statement>()) {
                hasDeclarations = true;
                break;
            }
        if (!hasDeclarations) return &statement->components;
    }

    if (statement->components.empty()) return new IR::EmptyStatement(statement->srcInfo);
    if (statement->components.size() == 1) {
        auto first = statement->components.at(0);
        if (first->is<IR::Statement>()) return first;
    }
    return statement;
}

const IR::Node *DoSimplifyControlFlow::postorder(IR::IfStatement *statement) {
    LOG3("Visiting " << dbp(getOriginal()));
    if (auto lnot = statement->condition->to<IR::LNot>()) {
        // swap branches
        statement->condition = lnot->expr;
        auto e = statement->ifFalse;
        if (!e) e = new IR::EmptyStatement();
        statement->ifFalse = statement->ifTrue;
        statement->ifTrue = e;
    }

    if (SideEffects::check(statement->condition, this, refMap, typeMap)) return statement;
    if (statement->ifTrue->is<IR::EmptyStatement>() &&
        (statement->ifFalse == nullptr || statement->ifFalse->is<IR::EmptyStatement>()))
        return new IR::EmptyStatement(statement->srcInfo);
    return statement;
}

const IR::Node *DoSimplifyControlFlow::postorder(IR::EmptyStatement *statement) {
    LOG3("Visiting " << dbp(getOriginal()));
    auto parent = findContext<IR::Statement>();
    if (parent == nullptr ||  // in a ParserState or P4Action
        parent->is<IR::BlockStatement>())
        // remove it from blocks
        return nullptr;
    return statement;
}

const IR::Node *DoSimplifyControlFlow::postorder(IR::SwitchStatement *statement) {
    LOG3("Visiting " << dbp(getOriginal()));
    if (statement->cases.empty()) {
        // If this is a table application remove the switch altogether but keep
        // the table application.  Otherwise remove the switch altogether.
        if (TableApplySolver::isActionRun(statement->expression, refMap, typeMap)) {
            auto mce = statement->expression->checkedTo<IR::Member>()
                           ->expr->checkedTo<IR::MethodCallExpression>();
            LOG2("Removing switch statement " << statement << " keeping " << mce);
            return new IR::MethodCallStatement(statement->srcInfo, mce);
        }
        if (SideEffects::check(statement->expression, this, refMap, typeMap))
            // This can happen if this pass is run before SideEffectOrdering.
            return statement;
        LOG2("Removing switch statement " << statement);
        return nullptr;
    }
    auto last = statement->cases.back();
    if (last->statement == nullptr) {
        auto it = statement->cases.rbegin();
        for (; it != statement->cases.rend(); ++it) {
            if ((*it)->statement != nullptr)
                break;
            else
                warn(ErrorType::WARN_MISSING, "%1%: fallthrough with no statement", last);
        }
        statement->cases.erase(it.base(), statement->cases.end());
    }
    return statement;
}

}  // namespace P4
