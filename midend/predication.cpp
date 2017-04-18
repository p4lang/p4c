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

#include "predication.h"

namespace P4 {

const IR::Node* Predication::postorder(IR::AssignmentStatement* statement) {
    if (!inside_action || ifNestingLevel == 0)
        return statement;
    auto right = new IR::Mux(predicate(), statement->right, statement->left);
    statement->right = right;
    return statement;
}

const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (!inside_action)
        return statement;

    ++ifNestingLevel;
    auto rv = new IR::BlockStatement;
    cstring conditionName = generator->newName("cond");
    auto condDecl = new IR::Declaration_Variable(conditionName, IR::Type::Boolean::get());
    rv->push_back(condDecl);
    auto condition = new IR::PathExpression(IR::ID(conditionName));

    // A vector for a new BlockStatement.
    auto block = new IR::BlockStatement;

    const IR::Expression* previousPredicate = predicate();  // This may be nullptr
    // a new name for the new predicate
    cstring newPredName = generator->newName("pred");
    predicateName.push_back(newPredName);
    auto decl = new IR::Declaration_Variable(newPredName, IR::Type::Boolean::get());
    block->push_back(decl);
    // This evaluates the if condition.
    // We are careful not to evaluate any conditional more times
    // than in the original program, since the evaluation may have side-effects.
    auto trueCond = new IR::AssignmentStatement(condition->clone(), statement->condition);
    block->push_back(trueCond);

    const IR::Expression* pred;
    if (previousPredicate == nullptr) {
        pred = condition->clone();
    } else {
        pred = new IR::LAnd(previousPredicate, condition->clone());
    }
    auto truePred = new IR::AssignmentStatement(predicate(), pred);
    block->push_back(truePred);

    visit(statement->ifTrue);
    block->push_back(statement->ifTrue);

    if (statement->ifFalse != nullptr) {
        auto neg = new IR::LNot(condition->clone());
        auto falseCond = new IR::AssignmentStatement(condition->clone(), neg);
        block->push_back(falseCond);
        if (previousPredicate == nullptr) {
            pred = condition->clone();
        } else {
            pred = new IR::LAnd(previousPredicate->clone(), condition->clone());
        }
        auto falsePred = new IR::AssignmentStatement(predicate(), pred);
        block->push_back(falsePred);

        visit(statement->ifFalse);
        block->push_back(statement->ifFalse);
    }

    rv->push_back(block);
    predicateName.pop_back();
    --ifNestingLevel;
    prune();
    return rv;
}

const IR::Node* Predication::preorder(IR::P4Action* action) {
    inside_action = true;
    return action;
}

const IR::Node* Predication::postorder(IR::P4Action* action) {
    inside_action = false;
    return action;
}

}  // namespace P4
