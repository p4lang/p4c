// SPDX-FileCopyrightText: 2022 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/common/lib/model.h"

#include <ostream>
#include <string>
#include <utility>

#include "frontends/p4/optimizeExpressions.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4::P4Tools {

Model::SubstVisitor::SubstVisitor(const Model &model, bool doComplete)
    : self(model), doComplete(doComplete) {}

const IR::Literal *Model::SubstVisitor::preorder(IR::StateVariable *var) {
    BUG("At this point all state variables should have been resolved. Encountered %1%.", var);
}

const IR::Literal *Model::SubstVisitor::preorder(IR::SymbolicVariable *var) {
    auto varIt = self.symbolicMap.find(var);
    if (varIt == self.symbolicMap.end()) {
        if (doComplete) {
            return IR::getDefaultValue(var->type, var->srcInfo, true)->checkedTo<IR::Literal>();
        }
        BUG("Variable not bound in model: %1%", var);
    }
    prune();
    return varIt->second->checkedTo<IR::Literal>();
}

const IR::Literal *Model::SubstVisitor::preorder(IR::TaintExpression *var) {
    return IR::getDefaultValue(var->type, var->getSourceInfo())->checkedTo<IR::Literal>();
}

IR::Ptr<IR::StructExpression> Model::evaluateStructExpr(const IR::StructExpression *structExpr,
                                                        bool doComplete,
                                                        ExpressionMap *resolvedExpressions) const {
    IR::MutablePtr<IR::StructExpression> resolvedStructExpr = structExpr->clone();
    resolvedStructExpr->components.clear();
    for (auto namedExpr : structExpr->components) {
        IR::Ptr<IR::Expression> resolvedExpr = nullptr;
        if (const auto *subStructExpr = namedExpr->expression->to<IR::StructExpression>()) {
            resolvedExpr = evaluateStructExpr(subStructExpr, doComplete, resolvedExpressions);
        } else if (const auto *subListExpr = namedExpr->expression->to<IR::BaseListExpression>()) {
            resolvedExpr = evaluateListExpr(subListExpr, doComplete, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(namedExpr->expression, doComplete, resolvedExpressions);
        }
        resolvedStructExpr->components.push_back(
            new IR::NamedExpression(namedExpr->srcInfo, namedExpr->name, resolvedExpr));
    }
    if (auto headerExpr = resolvedStructExpr->to<IR::HeaderExpression>()) {
        headerExpr->validity = evaluate(headerExpr->validity, doComplete);
    }
    return resolvedStructExpr;
}

IR::Ptr<IR::BaseListExpression> Model::evaluateListExpr(const IR::BaseListExpression *listExpr,
                                                        bool doComplete,
                                                        ExpressionMap *resolvedExpressions) const {
    IR::MutablePtr<IR::BaseListExpression> resolvedListExpr =
        new IR::BaseListExpression(listExpr->srcInfo, listExpr->type, {});
    for (auto expr : listExpr->components) {
        IR::Ptr<IR::Expression> resolvedExpr = nullptr;
        if (const auto *subListExpr = expr->to<IR::BaseListExpression>()) {
            resolvedExpr = evaluateListExpr(subListExpr, doComplete, resolvedExpressions);
        } else if (const auto *subStructExpr = expr->to<IR::StructExpression>()) {
            resolvedExpr = evaluateStructExpr(subStructExpr, doComplete, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(expr, doComplete, resolvedExpressions);
        }
        resolvedListExpr->components.push_back(resolvedExpr);
    }
    return resolvedListExpr;
}

IR::Ptr<IR::Literal> Model::evaluate(IR::Ptr<IR::Expression> expr, bool doComplete,
                                     ExpressionMap *resolvedExpressions) const {
    auto substituted = expr->apply(SubstVisitor(*this, doComplete));
    auto evaluated = P4::optimizeExpression(substituted);
    const auto *literal = evaluated->checkedTo<IR::Literal>();
    // Add the variable to the resolvedExpressions list, if the list is not null.
    if (resolvedExpressions != nullptr) {
        (*resolvedExpressions)[expr] = literal;
    }
    return literal;
}

const IR::Expression *Model::get(const IR::SymbolicVariable *var, bool checked) const {
    auto it = symbolicMap.find(var);
    if (it != symbolicMap.end()) {
        return it->second;
    }
    BUG_CHECK(!checked, "Unable to find var %s in the model.", var);
    return nullptr;
}

void Model::set(const IR::SymbolicVariable *var, const IR::Expression *val) {
    symbolicMap[var] = val;
}

const SymbolicMapping &Model::getSymbolicMap() const { return symbolicMap; }

void Model::mergeMap(const SymbolicMapping &sourceMap) {
    for (const auto &varTuple : sourceMap) {
        symbolicMap.emplace(varTuple.first, varTuple.second);
    }
}

}  // namespace P4::P4Tools
