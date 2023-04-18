#include "backends/p4tools/common/lib/symbolic_env.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/zombie.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

const IR::Expression *SymbolicEnv::get(const IR::StateVariable &var) const {
    auto it = map.find(var);
    if (it != map.end()) {
        return it->second;
    }
    BUG("Unable to find var %s in the symbolic environment.", var.toString());
}

bool SymbolicEnv::exists(const IR::StateVariable &var) const { return map.find(var) != map.end(); }

void SymbolicEnv::set(const IR::StateVariable &var, const IR::Expression *value) {
    map[var] = P4::optimizeExpression(value);
}

Model *SymbolicEnv::complete(const Model &model) const {
    // Produce a new model based on the input model
    // Add the variables contained in this environment and try to complete them.
    auto *newModel = new Model(model);
    newModel->complete(map);
    return newModel;
}

Model *SymbolicEnv::evaluate(const Model &model) const {
    // Produce a new model based on the input model
    return model.evaluate(map);
}

const IR::Expression *SymbolicEnv::subst(const IR::Expression *expr) const {
    /// Traverses the IR to perform substitution.
    class SubstVisitor : public Transform {
        const SymbolicEnv &symbolicEnv;

        const IR::Node *preorder(IR::StateVariable *var) override {
            prune();
            if (symbolicEnv.exists(*var)) {
                const auto *result = symbolicEnv.get(*var);
                // Sometimes the symbolic constant and its declaration in the environment are the
                // same. We check if they are equal and return the member instead.
                if (var->equiv(*result)) {
                    return var;
                }
                return result;
            }
            return var;
        }

     public:
        explicit SubstVisitor(const SymbolicEnv &symbolicEnv) : symbolicEnv(symbolicEnv) {}
    };

    return expr->apply(SubstVisitor(*this));
}

const SymbolicMapType &SymbolicEnv::getInternalMap() const { return map; }

bool SymbolicEnv::isSymbolicValue(const IR::Node *node) {
    // Parser states are symbolic values.
    if (node->is<IR::ParserState>()) {
        return true;
    }

    // All other symbolic values are P4 expressions.
    const auto *expr = node->to<IR::Expression>();
    if (expr == nullptr) {
        return false;
    }

    // Concrete constants and symbolic constants form the basis of symbolic values.
    //
    // Constants and BoolLiterals are concrete constants.
    if (expr->is<IR::Constant>()) {
        return true;
    }
    if (expr->is<IR::BoolLiteral>()) {
        return true;
    }
    // Tainted expressions are symbolic values.
    if (expr->is<IR::TaintExpression>()) {
        return true;
    }
    // Concolic variables are symbolic values.
    if (expr->is<IR::ConcolicVariable>()) {
        return true;
    }
    // DefaultExpresssions are symbolic values.
    if (expr->is<IR::DefaultExpression>()) {
        return true;
    }

    // Symbolic constants are references to fields under the struct p4t*zombie.const.
    if (const auto *member = expr->to<IR::StateVariable>()) {
        return Zombie::isSymbolicConst(*member);
    }

    // Symbolic values can be composed using several IR nodes.
    if (const auto *unary = expr->to<IR::Operation_Unary>()) {
        return (unary->is<IR::Neg>() || unary->is<IR::LNot>() || unary->is<IR::Cmpl>() ||
                unary->is<IR::Cast>()) &&
               isSymbolicValue(unary->expr);
    }
    if (const auto *binary = expr->to<IR::Operation_Binary>()) {
        if (binary->is<IR::ArrayIndex>()) {
            return isSymbolicValue(binary->right);
        }
        return (binary->is<IR::Add>() || binary->is<IR::Sub>() || binary->is<IR::Mul>() ||
                binary->is<IR::Div>() || binary->is<IR::Mod>() || binary->is<IR::Equ>() ||
                binary->is<IR::Neq>() || binary->is<IR::Lss>() || binary->is<IR::Leq>() ||
                binary->is<IR::Grt>() || binary->is<IR::Geq>() || binary->is<IR::LAnd>() ||
                binary->is<IR::LOr>() || binary->is<IR::BAnd>() || binary->is<IR::BOr>() ||
                binary->is<IR::BXor>() || binary->is<IR::Shl>() || binary->is<IR::Shr>() ||
                binary->is<IR::Concat>() || binary->is<IR::Mask>()) &&
               isSymbolicValue(binary->left) && isSymbolicValue(binary->right);
    }
    if (const auto *slice = expr->to<IR::Slice>()) {
        return isSymbolicValue(slice->e0) && isSymbolicValue(slice->e1) &&
               isSymbolicValue(slice->e2);
    }
    if (const auto *listExpr = expr->to<IR::ListExpression>()) {
        return std::all_of(
            listExpr->components.begin(), listExpr->components.end(),
            [](const IR::Expression *component) { return isSymbolicValue(component); });
    }
    if (const auto *structExpr = expr->to<IR::StructExpression>()) {
        return std::all_of(structExpr->components.begin(), structExpr->components.end(),
                           [](const IR::NamedExpression *component) {
                               return isSymbolicValue(component->expression);
                           });
    }

    return false;
}

}  // namespace P4Tools
