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

#include <algorithm>
#include <ostream>
#include <string>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "frontends/p4/cloner.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/log.h"

namespace P4 {

namespace Pred {

/// return null cstring if not a reference to a lvalue.
static cstring lvalueName(const IR::Expression *exp) {
    if (auto p = exp->to<IR::PathExpression>()) return p->path->name;
    if (auto m = exp->to<IR::Member>()) {
        if (auto base = lvalueName(m->expr)) return base + "." + m->member;
    } else if (auto a = exp->to<IR::ArrayIndex>()) {
        if (auto k = a->right->to<IR::Constant>()) {
            if (auto base = lvalueName(a->left))
                return base + "[" + std::to_string(k->asInt()) + "]";
        } else if (auto index = lvalueName(a->right)) {
            if (auto base = lvalueName(a->left)) return base + "[" + index + "]";
        }
    } else if (auto s = exp->to<IR::Slice>()) {
        if (auto base = lvalueName(s->e0))
            if (auto h = s->e1->to<IR::Constant>())
                if (auto l = s->e2->to<IR::Constant>())
                    return base + "." + std::to_string(h->asInt()) + ":" +
                           std::to_string(l->asInt());
    }
    return cstring();
}

}  // namespace Pred

const IR::Node *Predication::EmptyStatementRemover::postorder(IR::EmptyStatement *) {
    return nullptr;
}

const IR::Node *Predication::EmptyStatementRemover::postorder(IR::BlockStatement *statement) {
    if (statement->components.empty()) return nullptr;
    return statement;
}

/// Allows nesting of Mux expressions
const IR::Mux *Predication::ExpressionReplacer::preorder(IR::Mux *mux) {
    ++currentNestingLevel;
    LOG1("Visiting Mux expression: " << *mux << " on level: " << currentNestingLevel);
    bool thenElsePass = traversalPath[currentNestingLevel - 1];
    if (currentNestingLevel == traversalPath.size()) {
        emplaceExpression(mux);
    } else if (currentNestingLevel < traversalPath.size()) {
        visitBranch(mux, thenElsePass);
    }
    visit(mux->e1);
    visit(mux->e2);
    --currentNestingLevel;
    LOG1("Finished visiting Mux");
    return mux;
}

void Predication::ExpressionReplacer::setVisitingIndex(bool val) { visitingIndex = val; }

/// Right side of the statement is emplaced into the appropriate part of
/// the Mux expression, according to the IF/ELSE branch currently visited
void Predication::ExpressionReplacer::emplaceExpression(IR::Mux *mux) {
    auto condition = conditions[conditions.size() - currentNestingLevel];
    bool thenElsePass = traversalPath[currentNestingLevel - 1];
    mux->e0 = condition;
    if (thenElsePass) {
        mux->e1 = statement->right;
    } else {
        mux->e2 = statement->right;
    }
}

/// Here "visit" is recursively called on nested Mux expressions,
/// according to the current nesting level and also the structure of the 'mux' variable
void Predication::ExpressionReplacer::visitBranch(IR::Mux *mux, bool then) {
    auto condition = conditions[conditions.size() - currentNestingLevel - 1];
    auto leftName = Pred::lvalueName(statement->left);
    auto thenExprName = Pred::lvalueName(mux->e1);
    auto elseExprName = Pred::lvalueName(mux->e2);

    if (leftName.isNullOrEmpty()) {
        ::error(ErrorType::ERR_EXPRESSION,
                "%1%: Assignment inside if statement can't be transformed to condition expression",
                statement);
    }

    if (then && elseExprName == leftName && !visitingIndex) {
        mux->e2 = statement->left;
    } else if (!then && thenExprName == leftName && !visitingIndex) {
        mux->e1 = statement->left;
    }

    if (then) {
        if (mux->e1->is<IR::Mux>() || thenExprName.isNullOrEmpty() || thenExprName == leftName) {
            if (!mux->e1->is<IR::Mux>()) {
                if (visitingIndex) {
                    mux->e1 = new IR::Mux(condition,
                                          new IR::Constant(statement->right->type->getP4Type(), 0),
                                          new IR::Constant(statement->right->type->getP4Type(), 0));
                } else {
                    mux->e1 = new IR::Mux(condition, statement->left, statement->left);
                }
            }
            visit(mux->e1);
        }
    } else {
        if (mux->e2->is<IR::Mux>() || elseExprName.isNullOrEmpty() || elseExprName == leftName) {
            if (!mux->e2->is<IR::Mux>()) {
                if (visitingIndex) {
                    mux->e2 = new IR::Mux(condition,
                                          new IR::Constant(statement->right->type->getP4Type(), 0),
                                          new IR::Constant(statement->right->type->getP4Type(), 0));
                } else {
                    mux->e2 = new IR::Mux(condition, statement->left, statement->left);
                }
            }
            visit(mux->e2);
        }
    }
}

const IR::Expression *Predication::clone(const IR::Expression *expression) {
    // Expressions often need to be cloned. This is necessary because
    // in the end different code will be generated for the different clones of
    // an expression. This is most obvious if one clone is on the LHS and one
    // on the RHS of an assigment.
    ClonePathExpressions cloner;
    cloner.setCalledBy(this);
    return expression->apply(cloner);
}

const IR::Node *Predication::clone(const IR::AssignmentStatement *statement) {
    // Expressions often need to be cloned. This is necessary because
    // in the end different code will be generated for the different clones of
    // an expression.
    ClonePathExpressions cloner;
    cloner.setCalledBy(this);
    return statement->apply(cloner);
}

/// expressionReplacer is applied here and the assignment is stored in liveAssigns vector
const IR::Node *Predication::preorder(IR::AssignmentStatement *statement) {
    if (findContext<IR::P4Action>() == nullptr || ifNestingLevel == 0) {
        return statement;
    }
    LOG1("In preorder for statement: " << *statement);
    const Context *ctxt = nullptr;
    std::vector<const IR::Expression *> conditions;
    while (auto ifs = findContext<IR::IfStatement>(ctxt)) {
        conditions.push_back(ifs->condition);
    }
    // Checks to see if index modification is needed
    if (statement->left->is<IR::Member>() || statement->left->is<IR::ArrayIndex>()) {
        modifyIndex = true;
        visit(statement->left);
        modifyIndex = false;
    }
    // The expressionReplacer responsible for transforming this statement
    ExpressionReplacer replacer(clone(statement)->to<IR::AssignmentStatement>(), traversalPath,
                                conditions);
    replacer.setCalledBy(this);
    dependencies.clear();
    visit(statement->right);
    LOG2("Finished visiting right side of statement");
    // Go through all dependencies acquired from visiting the right side of the
    // statement and check if they are already in the liveAssignments map
    for (auto dependency : dependencies) {
        if (liveAssignments.find(dependency) != liveAssignments.end()) {
            // Save statement's name if it is dependent
            dependentNames.push_back(Pred::lvalueName(statement->left));
            // remove dependency from liveAssignments to not duplicate
            liveAssignments.erase(dependency);
            depNestingLevel = ifNestingLevel;
        }
    }
    auto statementName = Pred::lvalueName(statement->left);
    // Set value to true in isStatementDependent map
    // if the name of a current statement is the same as
    // the name of any in dependentNames.
    auto it = std::find(dependentNames.begin(), dependentNames.end(), statementName);
    if (it != dependentNames.end()) {
        isStatementDependent[statementName] = true;
    } else {
        isStatementDependent[statementName] = false;
    }
    // Push liveAssignments in liveAssigns in adequate order
    // If current statement is dependent, it should be pushed on liveAssigns.
    if (depNestingLevel < ifNestingLevel && isStatementDependent[statementName]) {
        statement->right = new IR::Mux(conditions.back(), statement->right, statement->left);
        if (!traversalPath[ifNestingLevel - 1]) {
            // change statement name, if the current branch is the 'else' branch
            statementName = generator->newName("elseStatement");
        }
    } else {
        // Search for a statement with the same name in liveAssignments, if it is there
        // then update the statement->right
        auto foundAssignment = liveAssignments.find(statementName);
        if (foundAssignment != liveAssignments.end()) {
            statement->right = foundAssignment->second->right;
        } else if (!statement->right->is<IR::Mux>()) {
            auto clonedLeft = clone(statement->left);
            statement->right = new IR::Mux(conditions.back(), clonedLeft, clonedLeft);
        }
        // Remove statement for 'then' if there is a statement
        // with the same statement name in the else branch.
        if (liveAssigns.size() > 0 && !isStatementDependent[statementName] &&
            Pred::lvalueName(liveAssigns.back()->left) == Pred::lvalueName(statement->left)) {
            liveAssigns.pop_back();
        }
    }

    // Push statement which now contains a Mux expression in the statement->right.
    // ExpressionReplacer is applied here which takes care of correct transforming.
    liveAssignments[statementName] =
        new IR::AssignmentStatement(statement->left, clone(statement->right)->apply(replacer));
    liveAssigns.push_back(liveAssignments[statementName]);
    LOG2("Finished visiting statement");
    LOG3("Pushed into liveAssigns statement: " << *(liveAssigns.back()));

    // Later removed by the emptyStatementRemover
    return new IR::EmptyStatement();
}

const IR::Node *Predication::preorder(IR::PathExpression *pathExpr) {
    dependencies.push_back(Pred::lvalueName(pathExpr));
    return pathExpr;
}
const IR::Node *Predication::preorder(IR::Member *member) {
    visit(member->expr);
    dependencies.push_back(Pred::lvalueName(member));
    return member;
}
const IR::Node *Predication::preorder(IR::ArrayIndex *arrInd) {
    visit(arrInd->left);
    // Collect conditions from IF-ELSE blocks surrounding this ArrayIndex
    const Context *ctxt = nullptr;
    std::vector<const IR::Expression *> conditions;
    while (auto ifs = findContext<IR::IfStatement>(ctxt)) {
        conditions.push_back(ifs->condition);
    }
    // The index modification isn't necessary if the index is a Constant
    if (!(arrInd->right->is<IR::Constant>()) && modifyIndex) {
        // Creates a new variable that has the value of the original index when the
        // condition is fulfilled, and in any other case it has a default value of '1w0'
        cstring indexName = generator->newName("index");
        auto indexDecl = new IR::Declaration_Variable(indexName, arrInd->right->type->getP4Type());
        auto index = new IR::PathExpression(IR::ID(indexName));
        auto indexAssignment = new IR::AssignmentStatement(index, clone(arrInd->right));
        ExpressionReplacer replacer(clone(indexAssignment)->to<IR::AssignmentStatement>(),
                                    traversalPath, conditions);
        // Creates the initial Mux expression
        replacer.setVisitingIndex(true);
        indexAssignment->right =
            new IR::Mux(conditions.back(), new IR::Constant(arrInd->right->type->getP4Type(), 0),
                        new IR::Constant(arrInd->right->type->getP4Type(), 0));
        // Applies the replacer and adds the declaration and assignment to vectors
        indexDeclarations.push_back(indexDecl);
        liveAssignments[indexName] = new IR::AssignmentStatement(
            indexAssignment->left, indexAssignment->right->apply(replacer));
        liveAssigns.push_back(liveAssignments[indexName]);

        arrInd->right = index;
    }
    dependencies.push_back(Pred::lvalueName(arrInd));
    return arrInd;
}

const IR::Node *Predication::preorder(IR::IfStatement *statement) {
    if (findContext<IR::P4Action>() == nullptr) {
        return statement;
    }
    ++ifNestingLevel;
    LOG1("Preorder of IfStatement, level: " << ifNestingLevel);
    LOG2(*statement);
    // rv block is the actual output of this preorder, it contains all of the newly
    // transformed statements, every new IF block has a new 'rv' block
    auto rv = new IR::BlockStatement;
    // If the condition is of composite nature then an 'alias' needs to be made. It's declaration
    // can be pushed onto 'rv' immediately, while it's assignment of value needs to be pushed
    // onto 'liveAssigns', delaying it's placement on the 'rv' block and avoiding ordering issues.
    if (!statement->condition->is<IR::PathExpression>()) {
        cstring conditionName = generator->newName("cond");
        rv->push_back(new IR::Declaration_Variable(conditionName, IR::Type::Boolean::get()));
        auto condition = new IR::PathExpression(IR::ID(conditionName));
        liveAssigns.push_back(new IR::AssignmentStatement(clone(condition), statement->condition));
        LOG1("Composite condition alias created: " << conditionName);
        LOG2(" " << statement->condition);
        statement->condition = condition;  // replace with variable cond
    }
    // Visit the IF block of this IF-ELSE statement
    traversalPath.push_back(true);
    visit(statement->ifTrue);
    LOG1("Finished visiting IF block");
    // By doing this recursive 'rv' block placement is achieved.
    // The same is valid for the block of code below which pushes statement->ifFalse.
    rv->push_back(statement->ifTrue);
    // This evaluates else branch
    if (statement->ifFalse != nullptr) {
        traversalPath.back() = false;
        visit(statement->ifFalse);
        LOG1("Finished visiting ELSE block");
        rv->push_back(statement->ifFalse);
    }
    // Push declarations on 'rv' block.
    for (auto it : indexDeclarations) {
        rv->push_back(it);
    }
    // Push assignments which are correctly aranged in liveAssigns vector on 'rv' block.
    for (auto it : liveAssigns) {
        rv->push_back(it);
    }
    indexDeclarations.clear();
    liveAssigns.clear();
    liveAssignments.clear();
    traversalPath.pop_back();
    LOG1("Finished visiting IF statement on level: " << ifNestingLevel);
    --ifNestingLevel;
    prune();
    // Remove all empty statements which are inside this 'rv' block
    remover.setCalledBy(this);
    return rv->apply(remover);
}

const IR::Node *Predication::preorder(IR::P4Action *action) {
    inside_action = true;
    return action;
}

const IR::Node *Predication::postorder(IR::P4Action *action) {
    inside_action = false;
    return action;
}

}  // namespace P4
