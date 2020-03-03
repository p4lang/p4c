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
/// convert an expression into a string that uniqely identifies the value referenced
/// return null cstring if not a reference to a constant thing.
static cstring expr_name(const IR::Expression *exp) {
    if (auto p = exp->to<IR::PathExpression>())
        return p->path->name;
    if (auto m = exp->to<IR::Member>()) {
        if (auto base = expr_name(m->expr))
            return base + "." + m->member;
    } else if (auto a = exp->to<IR::ArrayIndex>()) {
        if (auto k = a->right->to<IR::Constant>())
            if (auto base = expr_name(a->left))
                return base + "." + std::to_string(k->asInt()); }
    return cstring();
}

bool Predication::ReplaceChecker::preorder(const IR::AssignmentStatement * statement){
    if (!statement->right->is<IR::Mux>() ) {
        conflictDetected = true;
        return false;
    } 
    assignmentStatement = statement;
    visit(statement->right);
    return false;
}

bool Predication::ReplaceChecker::preorder(const IR::Mux * mux){
    ++currentNestingLevel;

    bool thenElsePass = travesalPath[currentNestingLevel - 1];

    if (currentNestingLevel == travesalPath.size()) {
        if (thenElsePass) {
            conflictDetected = expr_name(mux->e1) != expr_name(assignmentStatement->left);
        } else {
            conflictDetected = expr_name(mux->e2) != expr_name(assignmentStatement->left);
        }

        return false;
    }
    
    
    if (thenElsePass) {
        if ( expr_name(mux->e1) == expr_name(assignmentStatement->left) ) {
            return false;
        }

        if ( !mux->e1->is<IR::Mux>() ) {
            conflictDetected = true;
            return false;
        }

        visit(mux->e1);
    } else {
        if ( expr_name(mux->e2) == expr_name(assignmentStatement->left) ) {
            return false;
        }

        if ( !mux->e2->is<IR::Mux>() ) {
            conflictDetected = true;
            return false; 
        }

        visit(mux->e2);
    }
    --currentNestingLevel;

    return false;

}




const IR::AssignmentStatement * Predication::ExpressionReplacer::preorder(IR::AssignmentStatement * assignmentStatement){
    statement = assignmentStatement;

    if (!assignmentStatement->right->is<IR::Mux>() ) {
        assignmentStatement->right = new IR::Mux(conditions.front(), assignmentStatement->left, assignmentStatement->left);
    } 

    visit(assignmentStatement->right);

    return assignmentStatement;
}


const IR::Mux * Predication::ExpressionReplacer::preorder(IR::Mux * mux) {

    ++currentNestingLevel;

    bool thenElsePass = travesalPath[currentNestingLevel - 1];
    const IR::Expression * condition = conditions[currentNestingLevel - 1];

    if (currentNestingLevel == travesalPath.size()) {
        mux->e0 = condition;
        if (thenElsePass) {
            mux->e1 = rightExpression;
        } else {
            mux->e2 = rightExpression;
        }
    } else if (thenElsePass) {
        if (expr_name(mux->e2) == expr_name(statement->left)) {
            mux->e2 = statement->left;
        }

        if (mux->e1->is<IR::Mux>() || expr_name(mux->e1) == nullptr || expr_name(mux->e1) == expr_name(statement->left)) {
            if(!mux->e1->is<IR::Mux>()) {
                mux->e1 = new IR::Mux(condition, statement->left, statement->left);
            }
            visit(mux->e1);
        }
    } else {
        if (expr_name(mux->e1) == expr_name(statement->left)){
            mux->e1 = statement->left; 
        }

        if (mux->e2->is<IR::Mux>() || expr_name(mux->e2) == nullptr || expr_name(mux->e2) == expr_name(statement->left)) {
            if (!mux->e2->is<IR::Mux>()) {
                mux->e2 = new IR::Mux(condition, statement->left, statement->left);
            }
            visit(mux->e2);
        }
    }
    --currentNestingLevel;
    


    return mux;
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
    if (!inside_action || ifNestingLevel == 0 || nestedConditions.empty())
        return statement;

    const IR::Node * returningStatement = new IR::EmptyStatement();

    ExpressionReplacer replacer(clone(statement->right), travesalPath, nestedConditions);
    ReplaceChecker replaceChecker(travesalPath);
    
    auto statementName = expr_name(statement->left);
    auto foundedAssignment = assignments.find(statementName);

    if (foundedAssignment != assignments.end()) {
        auto oldExpressionRight = statement->right;

        statement->right = foundedAssignment->second;
        statement->apply(replaceChecker);

        if ( replaceChecker.isConflictDetected()) {
            returningStatement  = new IR::AssignmentStatement(statement->left, foundedAssignment->second);
            statement->right = oldExpressionRight;
        }
    }

    auto updatedStatement = statement->apply(replacer);

    assignments[statementName] = updatedStatement->to<IR::AssignmentStatement>()->right;
    liveAssignments[statementName] = new IR::AssignmentStatement(clone(statement->left), clone(updatedStatement->to<IR::AssignmentStatement>()->right));

    return returningStatement;
}



const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (!inside_action)
        return statement;
    

    ++ifNestingLevel;
    auto rv = new IR::BlockStatement;
    
    nestedConditions.push_back(statement->condition);
    travesalPath.push_back(true);

    visit(statement->ifTrue);
    rv->push_back(statement->ifTrue);

    // This evaluates else branch
    if (statement->ifFalse != nullptr) {
        travesalPath.back() = false;

        visit(statement->ifFalse);
        rv->push_back(statement->ifFalse);
    }

    for ( auto assignmentPair: liveAssignments ) {
        rv->push_back( assignmentPair.second);
    }
    assignments.clear();
    liveAssignments.clear();

    nestedConditions.pop_back();
    travesalPath.pop_back();
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
