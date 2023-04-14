#ifndef BACKENDS_P4TOOLS_COMMON_LIB_MODEL_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_MODEL_H_

#include <map>
#include <set>
#include <vector>

#include <boost/container/flat_map.hpp>
#include "backends/p4tools/common/lib/formulae.h"

#include "ir/ir.h"

namespace P4Tools {

/// Symbolic maps map a state variable to a IR::Expression.
using SymbolicMapType = boost::container::flat_map<StateVariable, const IR::Expression *>;

/// Represents a solution found by the solver. A model is a concretized form of a symbolic
/// environment. All the expressions in a Model must be of type IR::Literal.
class Model : public SymbolicMapType {
 public:
    // Maps an expression to its value in the model.
    using ExpressionMap = std::map<const IR::Expression *, const IR::Literal *>;

    /// Completes the model with the variables in the given expression. A variable needs to be
    /// completed if it is not present in the model computed by the solver that produced the model.
    /// This typically happens when a variable is not needed to solve a set of constraints.
    void complete(const IR::Expression *expr);

    /// Completes the model with the variables in the given list of expressions. A variable needs to
    /// be completed if it is not present in the model computed by the solver that produced the
    /// model. This typically happens when a variable is not needed to solve a set of constraints.
    void complete(const SymbolicMapType &inputMap);

    /// Adds the given set of variables to the model (if they do not exist already).
    /// If the variable does not exist, it is initialized to a default value.
    void complete(const std::set<StateVariable> &inputSet);

    /// Evaluates a P4 expression in the context of this model.
    ///
    /// A BUG occurs if the given expression refers to a variable that is not bound by this model.
    /// If the input list @param resolvedExpressions is not null, we also collect the resolved value
    /// of this expression.
    const IR::Literal *evaluate(const IR::Expression *expr,
                                ExpressionMap *resolvedExpressions = nullptr) const;

    // Evaluates a P4 StructExpression in the context of this model. Recursively calls into
    // @evaluate and substitutes all members of this list with a Value type.
    const IR::StructExpression *evaluateStructExpr(const IR::StructExpression *structExpr,
                                                   ExpressionMap *resolvedExpressions) const;

    // Evaluates a P4 ListExpression in the context of this model. Recursively calls into @evaluate
    // and substitutes all members of this list with a Value type.
    const IR::ListExpression *evaluateListExpr(const IR::ListExpression *listExpr,
                                               ExpressionMap *resolvedExpressions) const;

    /// Evaluates a collection of P4 expressions in the context of this model by calling @evaluate
    /// on each member of the collection.
    ///
    /// A BUG occurs if any expression in the given collection refers to a variable that is not
    /// bound by this model.
    /// If the input list @param resolvedExpressions is not null, we also collect the bound values
    /// of all the variables we have resolved within this expression.
    template <template <class...> class Collection>
    std::vector<const IR::Literal *> evaluateAll(
        const Collection<const IR::Expression *> *exprs,
        ExpressionMap *resolvedExpressions = nullptr) const {
        std::vector<const IR::Literal *> result(exprs->size());
        for (const auto *expr : *exprs) {
            result.push_back(evaluate(expr, resolvedExpressions));
        }
        return result;
    }

    /// Produces a new model in the context of this current model by calling @evaluate on each
    /// member of the input map. If the input map contains values that are already in this model,
    /// they will be overwritten. A BUG occurs if any expression in the given collection refers to a
    /// variable that is not bound by this model. If the input list @param resolvedExpressions is
    /// not null, we also collect the bound values of all the variables we have resolved within this
    /// expression.
    Model *evaluate(const SymbolicMapType &inputMap,
                    ExpressionMap *resolvedExpressions = nullptr) const;

    /// Tries to retrieve @param var from the model.
    /// If @param checked is true, this function throws a BUG if the variable can not be found.
    /// Otherwise, it returns a nullptr.
    const IR::Expression *get(const StateVariable &var, bool checked) const;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_MODEL_H_ */
