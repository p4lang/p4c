#include "backends/p4tools/common/lib/model.h"

#include <string>
#include <utility>
#include <vector>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools {

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

const IR::StructExpression *Model::evaluateStructExpr(const IR::StructExpression *structExpr,
                                                      bool doComplete,
                                                      ExpressionMap *resolvedExpressions) const {
    auto *resolvedStructExpr =
        new IR::StructExpression(structExpr->srcInfo, structExpr->type, structExpr->structType, {});
    for (const auto *namedExpr : structExpr->components) {
        const IR::Expression *resolvedExpr = nullptr;
        if (const auto *subStructExpr = namedExpr->expression->to<IR::StructExpression>()) {
            resolvedExpr = evaluateStructExpr(subStructExpr, doComplete, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(namedExpr->expression, doComplete, resolvedExpressions);
        }
        resolvedStructExpr->components.push_back(
            new IR::NamedExpression(namedExpr->srcInfo, namedExpr->name, resolvedExpr));
    }
    return resolvedStructExpr;
}

const IR::ListExpression *Model::evaluateListExpr(const IR::ListExpression *listExpr,
                                                  bool doComplete,
                                                  ExpressionMap *resolvedExpressions) const {
    auto *resolvedListExpr = new IR::ListExpression(listExpr->srcInfo, listExpr->type, {});
    for (const auto *expr : listExpr->components) {
        const IR::Expression *resolvedExpr = nullptr;
        if (const auto *subStructExpr = expr->to<IR::ListExpression>()) {
            resolvedExpr = evaluateListExpr(subStructExpr, doComplete, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(expr, doComplete, resolvedExpressions);
        }
        resolvedListExpr->components.push_back(resolvedExpr);
    }
    return resolvedListExpr;
}

const IR::Literal *Model::evaluate(const IR::Expression *expr, bool doComplete,
                                   ExpressionMap *resolvedExpressions) const {
    const auto *substituted = expr->apply(SubstVisitor(*this, doComplete));
    const auto *evaluated = P4::optimizeExpression(substituted);
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

}  // namespace P4Tools
