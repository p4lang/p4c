#ifndef COMMON_LIB_FORMULAE_H_
#define COMMON_LIB_FORMULAE_H_

#include <stdint.h>

#include <string>

#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools {

/// Provides common functionality for implementing a thin wrapper around a 'const Node*' to enforce
/// invariants on which forms of IR nodes can inhabit implementations of this type. Implementations
/// must provide a static repOk(const Node*) function.
template <class Self, class Node = IR::Expression>
class AbstractRepCheckedNode {
 protected:
    gsl::not_null<const Node*> node;

    // Implicit conversions to allow implementations to be treated like a Node*.
    operator const Node*() const { return node; }
    const Node& operator*() const { return *node; }
    const Node* operator->() const { return node; }

    /// Performs a checked cast. A BUG occurs if the cast fails.
    template <class T>
    static const T* checkedTo(const IR::Node* n) {
        const T* result = n->to<T>();
        BUG_CHECK(result, "Cast failed: %1% is not a %2%. It is a %3% instead.", n,
                  T::static_type_name(), n->node_type_name());
        return result;
    }

    /// @param classDesc a user-friendly description of the class, for reporting errors to the
    ///     user.
    explicit AbstractRepCheckedNode(const Node* node, std::string classDesc) : node(node) {
        BUG_CHECK(Self::repOk(node), "%1%: Not a valid %2%.", node, classDesc);
    }
};

/// Represents a reference to an object in a P4 program.
///
/// This is a thin wrapper around a 'const IR::Member*' to (1) enforce invariants on which forms of
/// Members can represent state variables and (2) enable the use of StateVariables as map keys.
///
/// A Member can represent a StateVariable exactly when its qualifying expression
/// (IR::Member::expr) either is a PathExpression or can represent a StateVariable.
class StateVariable : private AbstractRepCheckedNode<StateVariable, IR::Member> {
 public:
    /// Determines whether @expr can represent a StateVariable.
    static bool repOk(const IR::Expression* expr);

    // Implicit conversions.
    using AbstractRepCheckedNode::operator const IR::Member*;
    using AbstractRepCheckedNode::operator*;
    using AbstractRepCheckedNode::operator->;

    // Implements comparisons so that StateVariables can be used as map keys.
    bool operator<(const StateVariable& other) const;
    bool operator==(const StateVariable& other) const;

 private:
    // Returns a negative value if e1 < e2, zero if e1 == e2, and a positive value otherwise.
    // In these comparisons,
    //   * PathExpressions < Members.
    //   * PathExpressions are ordered on the name contained in their Paths.
    //   * Members are ordered first by their expressions, then by their member.
    static int compare(const IR::Expression* e1, const IR::Expression* e2);
    static int compare(const IR::Member* m1, const IR::Expression* e2);
    static int compare(const IR::Member* m1, const IR::Member* m2);
    static int compare(const IR::PathExpression* p1, const IR::Expression* e2);
    static int compare(const IR::PathExpression* p1, const IR::PathExpression* p2);

 public:
    /// Implicitly converts IR::Expression* to a StateVariable.
    StateVariable(const IR::Expression* expr);  // NOLINT(runtime/explicit)
};

/// Represents a constraint that can be shipped to and asserted within a solver.
// TODO: This should implement AbstractRepCheckedNode<Constraint>.
using Constraint = IR::Expression;

/// Represents a P4 constant value.
//
// Representations of values include the following:
//   - IR::Constant for int-like values
//   - IR::BoolLiteral
// The lowest common ancestor of these two is an IR::Literal.
using Value = IR::Literal;

}  // namespace P4Tools

#endif /* COMMON_LIB_FORMULAE_H_ */
