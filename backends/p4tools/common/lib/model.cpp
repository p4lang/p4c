#include "backends/p4tools/common/lib/model.h"

#include <ostream>
#include <utility>

#include "backends/p4tools/common/lib/ir.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/ordered_map.h"
#include "lib/safe_vector.h"

namespace P4Tools {

class CompleteVisitor : public Inspector {
    Model* model;

 public:
    bool preorder(const IR::Member* member) override {
        if (model->count(member) == 0) {
            LOG_FEATURE(
                "common", 5,
                "***** Did not find a binding for " << member << ". Autocompleting." << std::endl);
            const auto* type = member->type;
            model->emplace(member, IRUtils::getDefaultValue(type));
        }
        return false;
    }

    bool preorder(const IR::ConcolicVariable* var) override {
        auto stateVar = StateVariable(var->concolicMember);
        model->emplace(stateVar, IRUtils::getDefaultValue(var->type));
        return false;
    }

    explicit CompleteVisitor(Model* model) : model(model) {}
};

void Model::complete(const IR::Expression* expr) { expr->apply(CompleteVisitor(this)); }

void Model::complete(const std::set<StateVariable>& inputSet) {
    auto completitionVisitor = CompleteVisitor(this);
    for (const auto& var : inputSet) {
        var->apply(completitionVisitor);
    }
}

void Model::complete(const SymbolicMapType& inputMap) {
    for (const auto& inputTuple : inputMap) {
        const auto* expr = inputTuple.second;
        expr->apply(CompleteVisitor(this));
    }
}

const IR::StructExpression* Model::evaluateStructExpr(const IR::StructExpression* structExpr,
                                                      ExpressionMap* resolvedExpressions) const {
    auto* resolvedStructExpr =
        new IR::StructExpression(structExpr->srcInfo, structExpr->type, structExpr->structType, {});
    for (const auto* namedExpr : structExpr->components) {
        const IR::Expression* resolvedExpr = nullptr;
        if (const auto* subStructExpr = namedExpr->expression->to<IR::StructExpression>()) {
            resolvedExpr = evaluateStructExpr(subStructExpr, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(namedExpr->expression, resolvedExpressions);
        }
        resolvedStructExpr->components.push_back(
            new IR::NamedExpression(namedExpr->srcInfo, namedExpr->name, resolvedExpr));
    }
    return resolvedStructExpr;
}

const IR::ListExpression* Model::evaluateListExpr(const IR::ListExpression* listExpr,
                                                  ExpressionMap* resolvedExpressions) const {
    auto* resolvedListExpr = new IR::ListExpression(listExpr->srcInfo, listExpr->type, {});
    for (const auto* expr : listExpr->components) {
        const IR::Expression* resolvedExpr = nullptr;
        if (const auto* subStructExpr = expr->to<IR::ListExpression>()) {
            resolvedExpr = evaluateListExpr(subStructExpr, resolvedExpressions);
        } else {
            resolvedExpr = evaluate(expr, resolvedExpressions);
        }
        resolvedListExpr->components.push_back(resolvedExpr);
    }
    return resolvedListExpr;
}

const Value* Model::evaluate(const IR::Expression* expr, ExpressionMap* resolvedExpressions) const {
    class SubstVisitor : public Transform {
        const Model& self;

     public:
        const IR::Literal* preorder(IR::Member* member) override {
            BUG_CHECK(self.count(member), "Variable not bound in model: %1%", member);
            prune();
            return self.at(StateVariable(member))->checkedTo<IR::Literal>();
        }

        const IR::Literal* preorder(IR::TaintExpression* var) override {
            return IRUtils::getDefaultValue(var->type);
        }

        const IR::Literal* preorder(IR::ConcolicVariable* var) override {
            auto stateVar = StateVariable(var->concolicMember);
            BUG_CHECK(self.count(stateVar), "Variable not bound in model: %1%",
                      stateVar->toString());
            return self.at(stateVar)->checkedTo<IR::Literal>();
        }

        explicit SubstVisitor(const Model& model) : self(model) {}
    };
    const auto* substituted = expr->apply(SubstVisitor(*this));
    const auto* evaluated = IRUtils::optimizeExpression(substituted);
    const auto* literal = evaluated->checkedTo<IR::Literal>();
    // Add the variable to the resolvedExpressions list, if the list is not null.
    if (resolvedExpressions != nullptr) {
        (*resolvedExpressions)[expr] = literal;
    }
    return literal;
}

Model* Model::evaluate(const SymbolicMapType& inputMap, ExpressionMap* resolvedExpressions) const {
    auto* result = new Model(*this);
    for (const auto& inputTuple : inputMap) {
        auto name = inputTuple.first;
        const auto* expr = inputTuple.second;
        (*result)[name] = evaluate(expr, resolvedExpressions);
    }
    return result;
}

const IR::Expression* Model::get(const StateVariable& var, bool checked) const {
    auto it = find(var);
    if (it != end()) {
        return it->second;
    }
    BUG_CHECK(!checked, "Unable to find var %s in the model.", var->toString());
    return nullptr;
}

}  // namespace P4Tools