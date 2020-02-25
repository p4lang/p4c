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
#include "frontends/p4/cloner.h"
namespace P4 {

const IR::Mux * Predication::ExpressionReplacer::preorder(IR::Mux * mux) {
    bool thenElsePass = travesalPath[ifNestingLevel];
    ++ifNestingLevel;

    if (ifNestingLevel == travesalPath.size()) {
        if (thenElsePass) {
            mux->e1 = statement->right;
            mux->e2 = clone(statement->left);
        } else {
            mux->e1 = clone(statement->left);
            mux->e2 = statement->right;
        }

    } else {
        if (thenElsePass) {
            mux->e2 = clone(statement->left);
            visit(mux->e1);
        } else {
            mux->e1 = clone(statement->left);
            visit(mux->e2);
        }
    }
    return mux;
}

const IR::Expression* Predication::ExpressionReplacer::clone(const IR::Expression* expression) {
    // We often need to clone expressions.  This is necessary because
    // in the end we will generate different code for the different clones of
    // an expression.  This is most obvious if one clone is on the LHS and one
    // on the RHS of an assigment.
    ClonePathExpressions cloner;
    return expression->apply(cloner);
}


const IR::Expression* Predication::clone(const IR::Expression* expression) {
    // We often need to clone expressions.  This is necessary because
    // in the end we will generate different code for the different clones of
    // an expression.  This is most obvious if one clone is on the LHS and one
    // on the RHS of an assigment.
    ClonePathExpressions cloner;
    return expression->apply(cloner);
}


const IR::Node* Predication::preorder(IR::AssignmentStatement* statement) {
    if (!inside_action || ifNestingLevel == 0)
        return statement;

    auto replacer = new ExpressionReplacer(statement, travesalPath);
    auto right = clone(rootMuxCondition);
    statement->right = right->apply(*replacer);

    return statement;
}

void Predication::pushThenCondition(const IR::Expression * condition) {
    auto prevMuxCondition = currentMuxCondition;
    currentMuxCondition = new IR::Mux(condition, condition, condition);
    if (rootMuxCondition == nullptr) {
        rootMuxCondition = currentMuxCondition;
    } else {
        if (travesalPath.empty() || travesalPath.back()) {
            prevMuxCondition->e1 = currentMuxCondition;
        } else {
            prevMuxCondition->e2 = currentMuxCondition;
        }
    }
    nestedMuxes.push_back(currentMuxCondition);
}

void Predication::popCondition() {
    nestedMuxes.pop_back();

    if (nestedMuxes.empty()) {
        rootMuxCondition = nullptr;
        currentMuxCondition = nullptr;
    } else {
        currentMuxCondition = nestedMuxes.back();
        currentMuxCondition->e1 = currentMuxCondition->e0;
        currentMuxCondition->e2 = currentMuxCondition->e0;
    }
}

const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (!inside_action)
        return statement;

    ++ifNestingLevel;
    auto rv = new IR::BlockStatement;

    pushThenCondition(statement->condition);
    travesalPath.push_back(true);  // pushing after to enable root else branch

    visit(statement->ifTrue);
    rv->push_back(statement->ifTrue);

    // This evaluates else branch
    if (statement->ifFalse != nullptr) {
        travesalPath.back() = false;

        visit(statement->ifFalse);
        rv->push_back(statement->ifFalse);
    }

    travesalPath.pop_back();
    popCondition();
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
