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

const IR::Expression* Predication::clone(const IR::Expression* expression) {
    // We often need to clone expressions.  This is necessary because
    // in the end we will generate different code for the different clones of
    // an expression.  This is most obvious if one clone is on the LHS and one
    // on the RHS of an assigment.
    ClonePathExpressions cloner;
    return expression->apply(cloner);
}

const IR::Node* Predication::postorder(IR::AssignmentStatement* statement) {
    if (!inside_action || ifNestingLevel == 0)
        return statement;

    auto right = new IR::Mux(predicate(), statement->right, clone(statement->left));
    statement->right = right;
    return statement;
}

void Predication::pushCondition(const IR::Expression * condition) {
    if (nestedCondition == nullptr) {
        nestedCondition = condition;
    } else {
        nestedCondition = new IR::LAnd(condition, nestedCondition);
    }
    conditionsNumber++;
}

void Predication::popCondition() {
    if (conditionsNumber == 1) {
        nestedCondition = nullptr;
    } else {
        auto rightAndExpression = nestedCondition->to<IR::LAnd>()->right;
        nestedCondition = rightAndExpression;
    }

    conditionsNumber--;
}


const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (!inside_action)
        return statement;

    ++ifNestingLevel;
    auto rv = new IR::BlockStatement;

    // This evaluates the if condition.
    // We are careful not to evaluate any conditional more times
    // than in the original program, since the evaluation may have side-effects.
    pushCondition(statement->condition);
    cstring newPredName = generator->newName("pred");
    predicateName.push_back(newPredName);
    auto decl = new IR::Declaration_Variable(newPredName, IR::Type::Boolean::get());
    rv->push_back(decl);


    IR::AssignmentStatement * truePred;
    if (ifNestingLevel > 1) {
        truePred = new IR::AssignmentStatement(predicate(), new IR::LAnd(predicateBeforeLast(), statement->condition));
    } else {
        truePred = new IR::AssignmentStatement(predicate(), statement->condition);
    }
    rv->push_back(truePred);

    visit(statement->ifTrue);
    rv->push_back(statement->ifTrue);

  

    // This evaluates else branch
    if (statement->ifFalse != nullptr) {
        popCondition();
        auto neg = new IR::LNot(clone(statement->condition));
        pushCondition(neg);

        IR::AssignmentStatement * falsePred;
        if (ifNestingLevel > 1) {
            falsePred = new IR::AssignmentStatement(predicate(), new IR::LAnd(predicateBeforeLast(), neg));
        } else {
            falsePred = new IR::AssignmentStatement(predicate(), neg);
        }
        rv->push_back(falsePred);

        visit(statement->ifFalse);
        rv->push_back(statement->ifFalse);
    }

    popCondition();

    if (nestedCondition != nullptr) {
        auto restoreIfPred = new IR::AssignmentStatement(predicate(), predicateBeforeLast());
        rv->push_back(restoreIfPred);
    }



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
