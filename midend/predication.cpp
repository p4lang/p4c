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
    auto right = new IR::Mux(Util::SourceInfo(), predicate(), statement->right, statement->left);
    statement->right = right;
    return statement;
}

const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (!inside_action)
        return statement;

    ++ifNestingLevel;
    auto vec = new IR::IndexedVector<IR::StatOrDecl>();
    cstring conditionName = generator->newName("cond");
    auto condDecl = new IR::Declaration_Variable(
        Util::SourceInfo(), conditionName, IR::Annotations::empty,
        IR::Type::Boolean::get(), nullptr);
    vec->push_back(condDecl);
    auto condition = new IR::PathExpression(IR::ID(conditionName));

    // A vector for a new BlockStatement.
    auto blockVec = new IR::IndexedVector<IR::StatOrDecl>();

    const IR::Expression* previousPredicate = predicate(); // This may be nullptr
    // a new name for the new predicate
    cstring newPredName = generator->newName("pred");
    predicateName.push_back(newPredName);
    auto decl = new IR::Declaration_Variable(
        Util::SourceInfo(), newPredName, IR::Annotations::empty,
        IR::Type::Boolean::get(), nullptr);
    blockVec->push_back(decl);
    // This evaluates the if condition.
    // We are careful not to evaluate any conditional more times
    // than in the original program, since the evaluation may have side-effects.
    auto trueCond = new IR::AssignmentStatement(Util::SourceInfo(), condition->clone(),
                                                statement->condition);
    blockVec->push_back(trueCond);

    const IR::Expression* pred;
    if (previousPredicate == nullptr) {
        pred = condition->clone();
    } else {
        pred = new IR::LAnd(Util::SourceInfo(), previousPredicate, condition->clone());
    }
    auto truePred = new IR::AssignmentStatement(Util::SourceInfo(), predicate(), pred);
    blockVec->push_back(truePred);

    visit(statement->ifTrue);
    blockVec->push_back(statement->ifTrue);

    if (statement->ifFalse != nullptr) {
        auto neg = new IR::LNot(Util::SourceInfo(), condition->clone());
        auto falseCond = new IR::AssignmentStatement(Util::SourceInfo(), condition->clone(), neg);
        blockVec->push_back(falseCond);
        if (previousPredicate == nullptr) {
            pred = condition->clone();
        } else {
            pred = new IR::LAnd(Util::SourceInfo(), previousPredicate->clone(), condition->clone());
        }
        auto falsePred = new IR::AssignmentStatement(Util::SourceInfo(), predicate(), pred);
        blockVec->push_back(falsePred);

        visit(statement->ifFalse);
        blockVec->push_back(statement->ifFalse);
    }

    auto block = new IR::BlockStatement(Util::SourceInfo(), IR::Annotations::empty, blockVec);
    vec->push_back(block);
    predicateName.pop_back();
    --ifNestingLevel;
    prune();
    return new IR::BlockStatement(Util::SourceInfo(), IR::Annotations::empty, vec);
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
