#include "backends/p4tools/common/lib/symbolic_env.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/lib/model.h"
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
    BUG("Unable to find var %s in the symbolic environment.", var);
}

bool SymbolicEnv::exists(const IR::StateVariable &var) const { return map.find(var) != map.end(); }

void SymbolicEnv::set(const IR::StateVariable &var, const IR::Expression *value) {
    BUG_CHECK(value->type && !value->type->is<IR::Type_Unknown>(),
              "Cannot set value with unspecified type: %1%", value);
    map[var] = value;
}

const IR::Expression *SymbolicEnv::subst(const IR::Expression *expr) const {
    /// Traverses the IR to perform substitution.
    class SubstVisitor : public Transform {
        const SymbolicEnv &symbolicEnv;

        const IR::Node *preorder(IR::Member *member) override {
            prune();
            if (symbolicEnv.exists(member)) {
                const auto *result = symbolicEnv.get(member);
                // Sometimes the symbolic constant and its declaration in the environment are the
                // same. We check if they are equal and return the member instead.
                if (member->equiv(*result)) {
                    return member;
                }
                return result;
            }
            return member;
        }

        const IR::Node *preorder(IR::PathExpression *path) override {
            prune();
            if (symbolicEnv.exists(path)) {
                const auto *result = symbolicEnv.get(path);
                return result;
            }
            return path;
        }

     public:
        explicit SubstVisitor(const SymbolicEnv &symbolicEnv) : symbolicEnv(symbolicEnv) {}
    };

    return expr->apply(SubstVisitor(*this));
}

const SymbolicMapType &SymbolicEnv::getInternalMap() const { return map; }

bool SymbolicEnv::isSymbolicValue(const IR::Node *node) {
    // Check the obvious case first.
    if (node->is<IR::SymbolicVariable>()) {
        return true;
    }

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
    // Constants, StringLiterals, and BoolLiterals are concrete constants.
    if (expr->is<IR::Constant>()) {
        return true;
    }
    if (expr->is<IR::BoolLiteral>()) {
        return true;
    }
    if (expr->is<IR::StringLiteral>()) {
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
    // DefaultExpressions are symbolic values.
    if (expr->is<IR::DefaultExpression>()) {
        return true;
    }
    // InOut references are symbolic when the resolved input argument is symbolic.
    if (const auto *inout = expr->to<IR::InOutReference>()) {
        return isSymbolicValue(inout->resolvedRef);
    }
    // Symbolic values can be composed using several IR nodes.
    if (const auto *unary = expr->to<IR::Operation_Unary>()) {
        return (unary->is<IR::Neg>() || unary->is<IR::LNot>() || unary->is<IR::Cmpl>() ||
                unary->is<IR::Cast>()) &&
               isSymbolicValue(unary->expr);
    }
    if (const auto *binary = expr->to<IR::Operation_Binary>()) {
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
    if (const auto *listExpr = expr->to<IR::BaseListExpression>()) {
        return std::all_of(
            listExpr->components.begin(), listExpr->components.end(),
            [](const IR::Expression *component) { return isSymbolicValue(component); });
    }
    if (const auto *structExpr = expr->to<IR::StructExpression>()) {
        auto symbolicMembers =
            std::all_of(structExpr->components.begin(), structExpr->components.end(),
                        [](const IR::NamedExpression *component) {
                            return isSymbolicValue(component->expression);
                        });
        if (const auto *headerExpr = structExpr->to<IR::HeaderExpression>()) {
            auto isValid = isSymbolicValue(headerExpr->validity);
            return isValid && symbolicMembers;
        }
        return symbolicMembers;
    }

    return false;
}

}  // namespace P4Tools
