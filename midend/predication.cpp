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

const IR::Expression * Predication::aggregate() const{
    if (conditions.empty()) {
        return nullptr;
    }
    if ( conditions.size() == 1){
        return conditions.at(0);
    }
    const IR::Expression * predicate = nullptr;
    for ( auto cond: conditions ) {
        if ( predicate == nullptr ) {
            predicate = cond;
        } else {
            predicate = new IR::LAnd(cond , predicate);
        } 
    }
    return predicate;
}


const IR::Node* Predication::postorder(IR::AssignmentStatement* statement) {
    if (!inside_action || ifNestingLevel == 0)
        return statement;

    auto right = new IR::Mux(predicate(), statement->right, clone(statement->left));
    statement->right = right;
    return statement;
}

const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (!inside_action)
        return statement;

    ++ifNestingLevel;
    auto rv = new IR::BlockStatement;

    // This evaluates the if condition.
    // We are careful not to evaluate any conditional more times
    // than in the original program, since the evaluation may have side-effects.
    conditions.push_back(statement->condition);
    cstring newPredName = generator->newName("pred");
    predicateName.push_back(newPredName);
    auto decl = new IR::Declaration_Variable(newPredName, IR::Type::Boolean::get());
    rv->push_back(decl);

    if (!statement->ifTrue->is<IR::IfStatement>()) {
        const IR::Expression* pred = aggregate();
        auto truePred = new IR::AssignmentStatement(predicate(), pred);
        rv->push_back(truePred);
    }

    visit(statement->ifTrue);
    rv->push_back(statement->ifTrue);

    // This evaluates else branch 
    if (statement->ifFalse != nullptr) {
        // Poping last condition and applying logical not
        conditions.pop_back();
        auto neg = new IR::LNot(clone(statement->condition));
        conditions.push_back(neg);
        
        const IR::Expression* pred = aggregate();
       
        auto falsePred = new IR::AssignmentStatement(predicate(), pred);
        rv->push_back(falsePred);
       
        visit(statement->ifFalse);
        rv->push_back(statement->ifFalse);
    }

    conditions.pop_back();
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
