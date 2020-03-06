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
#include <algorithm>

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

bool Predication::ReplaceChecker::preorder(const IR::AssignmentStatement * statement) {
    if (!statement->right->is<IR::Mux>()) {
        conflictDetected = true;
        return false;
    }
    assignmentStatement = statement;
    visit(statement->right);
    return false;
}

bool Predication::ReplaceChecker::preorder(const IR::Mux * mux) {
    ++currentNestingLevel;
    auto leftAssignmentName = expr_name(assignmentStatement->left);
    bool thenElsePass = travesalPath[currentNestingLevel - 1];

    if (currentNestingLevel == travesalPath.size()) {
        if (thenElsePass) {
            conflictDetected = expr_name(mux->e1) != leftAssignmentName;
        } else {
            conflictDetected = expr_name(mux->e2) != leftAssignmentName;
        }

        return false;
    }

    if (thenElsePass) {
        // Visiting then
        if (expr_name(mux->e1) == leftAssignmentName) {
            return false;
        }

        if ( !mux->e1->is<IR::Mux>() ) {
            conflictDetected = true;
            return false;
        }

        visit(mux->e1);
    } else {
        // Visiting else
        if (expr_name(mux->e2) == leftAssignmentName) {
            return false;
        }

        if (!mux->e2->is<IR::Mux>()) {
            conflictDetected = true;
            return false;
        }

        visit(mux->e2);
    }
    --currentNestingLevel;

    return false;
}

IR::BlockStatement* Predication::EmptyStatementRemover::preorder(IR::BlockStatement * block) {
    for (auto iter = block->components.begin(); iter != block->components.end(); iter++) {
        if ((*iter)->is<IR::EmptyStatement>()) {
            block->components.erase(iter--);
        } else if ((*iter)->is<IR::BlockStatement>()) {
            visit(*iter);
            if ((*iter)->to<IR::BlockStatement>()->components.size() == 0) {
                block->components.erase(iter--);
            }
        }
    }

    return block;
}

const IR::AssignmentStatement *
Predication::ExpressionReplacer::preorder(IR::AssignmentStatement * as) {
    statement = as;

    if (!as->right->is<IR::Mux>()) {
        as->right = new IR::Mux(conditions.front(), as->left, as->left);
    }

    visit(as->right);

    return as;
}

void Predication::ExpressionReplacer::emplaceExpression(IR::Mux * mux) {
    const IR::Expression * condition = conditions[currentNestingLevel - 1];
    bool thenElsePass = travesalPath[currentNestingLevel - 1];

    mux->e0 = condition;
    if (thenElsePass) {
        mux->e1 = rightExpression;
    } else {
        mux->e2 = rightExpression;
    }
}

void Predication::ExpressionReplacer::visitThen(IR::Mux * mux) {
    const IR::Expression * condition = conditions[currentNestingLevel - 1];
    auto leftName = expr_name(statement->left);
    auto thenExprName = expr_name(mux->e1);

    if (expr_name(mux->e2) == expr_name(statement->left)) {
        mux->e2 = statement->left;
    }
    if (mux->e1->is<IR::Mux>() || thenExprName.isNullOrEmpty() || thenExprName == leftName) {
        if (!mux->e1->is<IR::Mux>()) {
            mux->e1 = new IR::Mux(condition, statement->left, statement->left);
        }
        visit(mux->e1);
    }
}

void Predication::ExpressionReplacer::visitElse(IR::Mux * mux) {
    const IR::Expression * condition = conditions[currentNestingLevel - 1];
    auto leftName = expr_name(statement->left);
    auto elseExprName = expr_name(mux->e2);

    if (expr_name(mux->e1) == leftName) {
        mux->e1 = statement->left;
            mux->e1 = statement->left;
        mux->e1 = statement->left;
    }
    if (mux->e2->is<IR::Mux>() || elseExprName.isNullOrEmpty() || elseExprName == leftName) {
        if (!mux->e2->is<IR::Mux>()) {
            mux->e2 = new IR::Mux(condition, statement->left, statement->left);
        }
        visit(mux->e2);
    }
}


const IR::Mux * Predication::ExpressionReplacer::preorder(IR::Mux * mux) {
    ++currentNestingLevel;
    bool thenElsePass = travesalPath[currentNestingLevel - 1];

    if (currentNestingLevel == travesalPath.size()) {
        emplaceExpression(mux);
    } else if (thenElsePass) {
        visitThen(mux);
    } else {
        visitElse(mux);
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

const IR::AssignmentStatement* Predication::clone(const IR::AssignmentStatement* statement) {
    // We often need to clone expressions.  This is necessary because
    // in the end we will generate different code for the different clones of
    // an expression.  This is most obvious if one clone is on the LHS and one
    // on the RHS of an assigment.
    ClonePathExpressions cloner;
    auto clonedAsssignment = new IR::AssignmentStatement(*statement);
    clonedAsssignment->left->apply(cloner);
    clonedAsssignment->right->apply(cloner);
    return clonedAsssignment;
}

const IR::Node* Predication::preorder(IR::AssignmentStatement* statement) {
    if (!inside_action || ifNestingLevel == 0 || nestedConditions.empty())
        return statement;

    ExpressionReplacer replacer(clone(statement->right), travesalPath, nestedConditions);
    ReplaceChecker replaceChecker(travesalPath);

    // Referenced lvalue is not appeard before
    dependecies.clear();
    visit(statement->right);

    // print out dependecies
    for (auto dependency : dependecies) {
        if (liveAssignments.find(dependency) != liveAssignments.end()) {
            // print out dependecy
            currentBlock->push_back(liveAssignments[dependency]);
            // remove from names to not duplicate
            orderedNames.erase(std::remove(orderedNames.begin(), orderedNames.end(), dependency));
            liveAssignments.erase(dependency);
        }
    }

    auto statementName = expr_name(statement->left);
    auto foundedAssignment = liveAssignments.find(statementName);

    if (foundedAssignment != liveAssignments.end()) {
        auto oldExpressionRight = statement->right;

        statement->right = foundedAssignment->second->right;
        statement->apply(replaceChecker);

        if (replaceChecker.isConflictDetected()) {
            currentBlock->push_back(clone(statement));
            statement->right = oldExpressionRight;
        }

        // move the lvalue assignment to the back
        orderedNames.erase(std::remove(orderedNames.begin(), orderedNames.end(), statementName));
    }

    orderedNames.push_back(statementName);

    auto updatedStatement = statement->apply(replacer);
    auto rightStatement = clone(updatedStatement->to<IR::AssignmentStatement>()->right);
    liveAssignments[statementName] = new IR::AssignmentStatement(statement->left, rightStatement);

    return new IR::EmptyStatement();
}

const IR::Node* Predication::preorder(IR::PathExpression * pathExpr) {
    dependecies.push_back(expr_name(pathExpr));
    return pathExpr;
}
const IR::Node* Predication::preorder(IR::Member * member) {
    visit(member->expr);
    dependecies.push_back(expr_name(member));
    return member;
}
const IR::Node* Predication::preorder(IR::ArrayIndex * arrInd) {
    visit(arrInd->left);
    dependecies.push_back(expr_name(arrInd));
    return arrInd;
}

const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (!inside_action)
        return statement;

    ++ifNestingLevel;
    auto rv = new IR::BlockStatement;
    currentBlock = rv;

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

    for (auto exprName : orderedNames) {
        rv->push_back(liveAssignments[exprName]);
    }
    liveAssignments.clear();
    orderedNames.clear();

    nestedConditions.pop_back();
    travesalPath.pop_back();
    --ifNestingLevel;

    prune();

    return rv->apply(remover);
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
