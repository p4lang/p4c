#include "backends/p4tools/common/lib/model.h"

#include <ostream>
#include <string>
#include <utility>

#include <boost/container/vector.hpp>

#include "frontends/p4/optimizeExpressions.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools {

Model::SubstVisitor::SubstVisitor(const Model &model) : self(model) {}

const IR::Literal *Model::SubstVisitor::preorder(IR::StateVariable *var) {
    BUG("At this point all state variables should have been resolved. Encountered %1%.", var);
}

const IR::Literal *Model::SubstVisitor::preorder(IR::SymbolicVariable *var) {
    BUG_CHECK(self.symbolicMap.find(var) != self.symbolicMap.end(),
              "Variable not bound in model: %1%", var);
    prune();
    return self.symbolicMap.at(var)->checkedTo<IR::Literal>();
}

const IR::Literal *Model::SubstVisitor::preorder(IR::TaintExpression *var) {
    return IR::getDefaultValue(var->type);
}

Model::CompleteVisitor::CompleteVisitor(Model &model) : self(model) {}

bool Model::CompleteVisitor::preorder(const IR::SymbolicVariable *var) {
    if (self.symbolicMap.find(var) == self.symbolicMap.end()) {
        LOG_FEATURE("common", 5,
                    "***** Did not find a binding for " << var << ". Autocompleting." << std::endl);
        const auto *type = var->type;
        self.symbolicMap.emplace(var, IR::getDefaultValue(type));
    }
    return false;
}

bool Model::CompleteVisitor::preorder(const IR::StateVariable *var) {
    BUG("At this point all state variables should have been resolved. Encountered %1%.", var);
}

void Model::complete(const IR::Expression *expr) { expr->apply(CompleteVisitor(*this)); }

void Model::complete(const SymbolicSet &inputSet) {
    auto completionVisitor = CompleteVisitor(*this);
    for (const auto &var : inputSet) {
        var->apply(completionVisitor);
    }
}

void Model::complete(const SymbolicMapType &inputMap) {
    for (const auto &inputTuple : inputMap) {
        const auto *expr = inputTuple.second;
        expr->apply(CompleteVisitor(*this));
    }
}

const IR::StructExpression *Model::evaluateStructExpr(const IR::StructExpression *structExpr,
                                                      ExpressionMap *resolvedExpressions) const {
    auto *resolvedStructExpr =
        new IR::StructExpression(structExpr->srcInfo, structExpr->type, structExpr->structType, {});
    for (const auto *namedExpr : structExpr->components) {
        const IR::Expression *resolvedExpr = nullptr;
        if (const auto *subStructExpr = namedExpr->expression->to<IR::StructExpression>()) {
            resolvedExpr = evaluateStructExpr(subStructExpr, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(namedExpr->expression, resolvedExpressions);
        }
        resolvedStructExpr->components.push_back(
            new IR::NamedExpression(namedExpr->srcInfo, namedExpr->name, resolvedExpr));
    }
    return resolvedStructExpr;
}

const IR::ListExpression *Model::evaluateListExpr(const IR::ListExpression *listExpr,
                                                  ExpressionMap *resolvedExpressions) const {
    auto *resolvedListExpr = new IR::ListExpression(listExpr->srcInfo, listExpr->type, {});
    for (const auto *expr : listExpr->components) {
        const IR::Expression *resolvedExpr = nullptr;
        if (const auto *subStructExpr = expr->to<IR::ListExpression>()) {
            resolvedExpr = evaluateListExpr(subStructExpr, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(expr, resolvedExpressions);
        }
        resolvedListExpr->components.push_back(resolvedExpr);
    }
    return resolvedListExpr;
}

const IR::Literal *Model::evaluate(const IR::Expression *expr,
                                   ExpressionMap *resolvedExpressions) const {
    const auto *substituted = expr->apply(SubstVisitor(*this));
    const auto *evaluated = P4::optimizeExpression(substituted);
    const auto *literal = evaluated->checkedTo<IR::Literal>();
    // Add the variable to the resolvedExpressions list, if the list is not null.
    if (resolvedExpressions != nullptr) {
        (*resolvedExpressions)[expr] = literal;
    }
    return literal;
}

Model *Model::evaluate(const SymbolicMapType &inputMap, ExpressionMap *resolvedExpressions) const {
    auto *result = new Model(*this);
    for (const auto &inputTuple : inputMap) {
        auto name = inputTuple.first;
        const auto *expr = inputTuple.second;
        (*result)[name] = evaluate(expr, resolvedExpressions);
    }
    return result;
}

const IR::Expression *Model::get(const IR::StateVariable &var, bool checked) const {
    auto it = find(var);
    if (it != end()) {
        return it->second;
    }
    BUG_CHECK(!checked, "Unable to find var %s in the model.", var->toString());
    return nullptr;
}

}  // namespace P4Tools
