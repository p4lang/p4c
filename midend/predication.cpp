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
/// convert an expression into a string that uniqely identifies the lvalue referenced
/// return null cstring if not a reference to a lvalue.
static cstring lvalue_name(const IR::Expression *exp) {
    if (auto p = exp->to<IR::PathExpression>())
        return p->path->name;
    if (auto m = exp->to<IR::Member>()) {
        if (auto base = lvalue_name(m->expr))
            return base + "." + m->member;
    } else if (auto a = exp->to<IR::ArrayIndex>()) {
        if (auto k = a->right->to<IR::Constant>())
            if (auto base = lvalue_name(a->left))
                return base + "." + std::to_string(k->asInt());
    } else if (auto s = exp->to<IR::Slice>()) {
        if (auto base = lvalue_name(s->e0))
            if (auto h = s->e1->to<IR::Constant>())
                if (auto l = s->e2->to<IR::Constant>())
                    return base + "." +
                        std::to_string(h->asInt()) + ":" + std::to_string(l->asInt());
    }
    return cstring();
}

bool Predication::EmptyStatementRemover::preorder(IR::BlockStatement * block) {
    for (auto iter = block->components.begin(); iter != block->components.end();) {
        if ((*iter)->is<IR::EmptyStatement>()) {
            iter = block->components.erase(iter);
        } else if ((*iter)->is<IR::BlockStatement>()) {
            visit(*iter);
            if ((*iter)->to<IR::BlockStatement>()->components.size() == 0) {
                iter = block->components.erase(iter);
            } else {
                iter++;
            }
        } else {
            iter++;
        }
    }
    return false;
}

const IR::Mux * Predication::ExpressionReplacer::preorder(IR::Mux * mux) {
    ++currentNestingLevel;
    bool thenElsePass = travesalPath[currentNestingLevel - 1];
    if (currentNestingLevel == travesalPath.size()) {
        emplaceExpression(mux);
    } else if (currentNestingLevel < travesalPath.size()) {
        visitBranch(mux, thenElsePass);
    }
    visit(mux->e1);
    visit(mux->e2);
    --currentNestingLevel;
    return mux;
}

void Predication::ExpressionReplacer::emplaceExpression(IR::Mux * mux) {
    auto condition = conditions[conditions.size() - currentNestingLevel];
    bool thenElsePass = travesalPath[currentNestingLevel - 1];
    mux->e0 = condition;
    if (thenElsePass) {
        mux->e1 = statement->right;
    } else {
        mux->e2 = statement->right;
    }
}

void Predication::ExpressionReplacer::visitBranch(IR::Mux * mux, bool then) {
    auto condition = conditions[conditions.size() - currentNestingLevel - 1];
    auto leftName = lvalue_name(statement->left);
    auto thenExprName = lvalue_name(mux->e1);
    auto elseExprName = lvalue_name(mux->e2);

    if (leftName.isNullOrEmpty()) {
        ::error(ErrorType::ERR_EXPRESSION,
                "%1%: Assignment inside if statement can't be transformed to condition expression",
                statement);
    }

    if (then && elseExprName == leftName) {
        mux->e2 = statement->left;
    } else if (!then && thenExprName == leftName) {
        mux->e1 = statement->left;
    }

    if (then) {
        if (mux->e1->is<IR::Mux>() || thenExprName.isNullOrEmpty() || thenExprName == leftName) {
            if (!mux->e1->is<IR::Mux>()) {
                mux->e1 = new IR::Mux(condition, statement->left, statement->left);
            }
            visit(mux->e1);
        }
    } else {
        if (mux->e2->is<IR::Mux>() || elseExprName.isNullOrEmpty() || elseExprName == leftName) {
            if (!mux->e2->is<IR::Mux>()) {
                mux->e2 = new IR::Mux(condition, statement->left, statement->left);
            }
            visit(mux->e2);
        }
    }
}

const IR::Expression* Predication::clone(const IR::Expression* expression) {
    // We often need to clone expressions.  This is necessary because
    // in the end we will generate different code for the different clones of
    // an expression.  This is most obvious if one clone is on the LHS and one
    // on the RHS of an assigment.
    ClonePathExpressions cloner;
    return expression->apply(cloner);
}

const IR::Node* Predication::clone(const IR::AssignmentStatement* statement) {
    // We often need to clone assignments.  This is necessary because
    // in the end we will generate different code for the different clones of
    // an assignments.
    ClonePathExpressions cloner;
    return statement->apply(cloner);
}

const IR::Node* Predication::preorder(IR::AssignmentStatement* statement) {
    if (findContext<IR::P4Action>() == nullptr || ifNestingLevel == 0) {
        return statement;
    }
    const Context * ctxt = nullptr;
    std::vector<const IR::Expression*> conditions;
    while (auto ifs = findContext<IR::IfStatement>(ctxt)) {
        conditions.push_back(ifs->condition);
    }
    auto clonedStatement = clone(statement)->to<IR::AssignmentStatement>();
    ExpressionReplacer replacer(clonedStatement, travesalPath, conditions);
    // Referenced lvalue is not appeard before
    dependencies.clear();
    visit(statement->right);
    // print out dependencies
    for (auto dependency : dependencies) {
        if (liveAssignments.find(dependency) != liveAssignments.end()) {
            // print out dependecy
            blocks.back()->push_back(liveAssignments[dependency]);
            dependencyAssignment = liveAssignments[dependency];
            // remove from names to not duplicate
            orderedNames.erase(dependency);
            liveAssignments.erase(dependency);
            depNestingLevel = ifNestingLevel;
            dependantName = lvalue_name(statement->left);
        }
    }
    auto statementName = lvalue_name(statement->left);
    statNames.push_back(statementName);
    bool isStatementDependant = false;
    for (auto it  = statNames.begin(); it != statNames.end(); it++) {
        if (dependantName == *it) {
            isStatementDependant = true;
        }
    }
    if (depNestingLevel < ifNestingLevel && isStatementDependant) {
        statement->right = new IR::Mux(conditions.back(), statement->right, statement->left);
        if (travesalPath[ifNestingLevel - 1]) {
            orderedNames.push_back(statementName);
            auto rightStatement = clone(statement->right)->apply(replacer);
            liveAssignments[statementName] =
                new IR::AssignmentStatement(statement->left, rightStatement);
        } else {
            cstring elseStatementsName = generator->newName("elseStatement");
            orderedNames.push_back(elseStatementsName);
            auto rightStatement = clone(statement->right)->apply(replacer);
            liveAssignments[elseStatementsName] =
                new IR::AssignmentStatement(statement->left, rightStatement);
        }
    } else {
        auto foundedAssignment = liveAssignments.find(statementName);
        if (foundedAssignment != liveAssignments.end()) {
            statement->right = foundedAssignment->second->right;
            // move the lvalue assignment to the back
            orderedNames.erase(statementName);
        } else if (!statement->right->is<IR::Mux>()) {
            auto clonedLeft = clone(statement->left);
            statement->right = new IR::Mux(conditions.back(), clonedLeft, clonedLeft);
        }
        orderedNames.push_back(statementName);
        auto rightStatement = clone(statement->right)->apply(replacer);
        liveAssignments[statementName] =
            new IR::AssignmentStatement(statement->left, rightStatement);
    }
    return new IR::EmptyStatement();
}

const IR::Node* Predication::preorder(IR::PathExpression * pathExpr) {
    dependencies.push_back(lvalue_name(pathExpr));
    return pathExpr;
}
const IR::Node* Predication::preorder(IR::Member * member) {
    visit(member->expr);
    dependencies.push_back(lvalue_name(member));
    return member;
}
const IR::Node* Predication::preorder(IR::ArrayIndex * arrInd) {
    visit(arrInd->left);
    dependencies.push_back(lvalue_name(arrInd));
    return arrInd;
}

const IR::Node* Predication::preorder(IR::IfStatement* statement) {
    if (findContext<IR::P4Action>() == nullptr) {
        return statement;
    }
    ++ifNestingLevel;
    auto rv = new IR::BlockStatement;
    // This pushes dependencies before IfStatement
    if (dependencyAssignment) {
        rv->push_back(blocks.back());
        blocks.pop_back();
        for (auto exprName : orderedNames) {
            if (!isAssignmentPushed[liveAssignments[exprName]]) {
                rv->push_back(liveAssignments[exprName]);
            }
            isAssignmentPushed[liveAssignments[exprName]] = true;
        }
    }
    if (!statement->condition->is<IR::PathExpression>()) {
        cstring conditionName = generator->newName("cond");
        auto condDecl = new IR::Declaration_Variable(conditionName, IR::Type::Boolean::get());
        rv->push_back(condDecl);
        auto condition = new IR::PathExpression(IR::ID(conditionName));
        rv->push_back(new IR::AssignmentStatement(clone(condition), statement->condition));
        statement->condition = condition;  // replace with variable cond
    }
    blocks.push_back(new IR::BlockStatement);
    travesalPath.push_back(true);
    visit(statement->ifTrue);
    rv->push_back(statement->ifTrue);
    // This evaluates else branch
    if (statement->ifFalse != nullptr) {
        travesalPath.back() = false;
        visit(statement->ifFalse);
        rv->push_back(statement->ifFalse);
    }
    rv->push_back(blocks.back());
    blocks.pop_back();
    blocks.push_back(new IR::BlockStatement);
    for (auto exprName : orderedNames) {
        if (!isAssignmentPushed[liveAssignments[exprName]]) {
            rv->push_back(liveAssignments[exprName]);
        }
        isAssignmentPushed[liveAssignments[exprName]] = false;
    }
    dependencyAssignment = nullptr;
    liveAssignments.clear();
    orderedNames.clear();
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
