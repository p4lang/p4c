/*
Copyright 2018-present Barefoot Networks, Inc.

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

#ifndef IR_PATTERN_H_
#define IR_PATTERN_H_

#include "ir/ir.h"

namespace P4 {

/**
 * Pattern matcher for IR::Expression trees.
 *
 * Build pattern expressions with `Pattern` to match subtrees or leaves, and combine
 * them with C++ operators to build larger patterns that can match tree fragments
 */
class Pattern {
    class Base {
     public:
        virtual bool match(const IR::Node *) = 0;
    } *pattern;
    Pattern(Base *p) : pattern(p) {}  // NOLINT(runtime/explicit)

    template <class T>
    class MatchExt : public Base {
        const T *&m;

     public:
        bool match(const IR::Node *n) override { return (m = n->to<T>()); }
        MatchExt(const T *&m) : m(m) {}  // NOLINT(runtime/explicit)
    };

    class Const : public Base {
        big_int value;

     public:
        bool match(const IR::Node *n) override {
            if (auto k = n->to<IR::Constant>()) return k->value == value;
            return false;
        }
        Const(big_int v) : value(v) {}  // NOLINT(runtime/explicit)
        Const(int v) : value(v) {}      // NOLINT(runtime/explicit)
    };
    template <class T>
    class Unary : public Base {
        Base *expr;

     public:
        bool match(const IR::Node *n) override {
            if (auto b = n->to<T>()) return expr->match(b->expr);
            return false;
        }
        Unary(Base *e) : expr(e) {}  // NOLINT(runtime/explicit)
    };
    template <class T>
    class Binary : public Base {
        Base *left, *right;
        bool commutative;

     public:
        bool match(const IR::Node *n) override {
            if (auto b = n->to<T>()) {
                if (left->match(b->left) && right->match(b->right)) return true;
                if (commutative && left->match(b->right) && right->match(b->left)) return true;
            }
            return false;
        }
        Binary(Base *l, Base *r, bool commute = false) : left(l), right(r), commutative(commute) {}
    };

 public:
    template <class T>
    class Match : public Base {
        const T *m;

     public:
        bool match(const IR::Node *n) override { return (m = n->to<T>()); }
        Match() : m(nullptr) {}
        const T *operator->() const { return m; }
        operator const T *() const { return m; }  // NOLINT(runtime/explicit)
        Pattern operator*(const Pattern &a) { return Pattern(*this) * a; }
        Pattern operator/(const Pattern &a) { return Pattern(*this) / a; }
        Pattern operator%(const Pattern &a) { return Pattern(*this) % a; }
        Pattern operator+(const Pattern &a) { return Pattern(*this) + a; }
        Pattern operator-(const Pattern &a) { return Pattern(*this) - a; }
        Pattern operator<<(const Pattern &a) { return Pattern(*this) << a; }
        Pattern operator>>(const Pattern &a) { return Pattern(*this) >> a; }
        Pattern operator==(const Pattern &a) { return Pattern(*this) == a; }
        Pattern operator!=(const Pattern &a) { return Pattern(*this) != a; }
        Pattern operator<(const Pattern &a) { return Pattern(*this) < a; }
        Pattern operator<=(const Pattern &a) { return Pattern(*this) <= a; }
        Pattern operator>(const Pattern &a) { return Pattern(*this) > a; }
        Pattern operator>=(const Pattern &a) { return Pattern(*this) >= a; }
        Pattern Relation(const Pattern &a) { return Pattern(*this).Relation(a); }
        Pattern operator&(const Pattern &a) { return Pattern(*this) & a; }
        Pattern operator|(const Pattern &a) { return Pattern(*this) | a; }
        Pattern operator^(const Pattern &a) { return Pattern(*this) ^ a; }
        Pattern operator&&(const Pattern &a) { return Pattern(*this) && a; }
        Pattern operator||(const Pattern &a) { return Pattern(*this) || a; }
        // avoid ambiguous overloads with operator const T * above
        Pattern operator+(int a) { return Pattern(*this) + Pattern(a); }
        Pattern operator-(int a) { return Pattern(*this) - Pattern(a); }
        Pattern operator==(int a) { return Pattern(*this) == Pattern(a); }
        Pattern operator!=(int a) { return Pattern(*this) != Pattern(a); }
    };

    template <class T = IR::AssignmentStatement>
    class Assign : public Base {
        static_assert(std::is_base_of_v<IR::BaseAssignmentStatement, T>);
        Base *left, *right;

     public:
        bool match(const IR::Node *n) override {
            if (auto as = n->to<T>()) {
                if (left->match(as->left) && right->match(as->right)) return true;
            }
            return false;
        }
        Assign(Base *l, Base *r) : left(l), right(r) {}
        Assign(const Pattern &l, const Pattern &r) : left(l.pattern), right(r.pattern) {}
        Assign(Base *l, int val) : left(l), right(new Const(val)) {}
        Assign(Base *l, big_int val) : left(l), right(new Const(val)) {}
    };

    template <class T>
    Pattern(const T *&m) : pattern(new MatchExt<T>(m)) {}  // NOLINT(runtime/explicit)
    template <class T>
    Pattern(Match<T> &m) : pattern(&m) {}                   // NOLINT(runtime/explicit)
    explicit Pattern(big_int v) : pattern(new Const(v)) {}  // NOLINT(runtime/explicit)
    explicit Pattern(int v) : pattern(new Const(v)) {}      // NOLINT(runtime/explicit)
    Pattern operator-() const { return Pattern(new Unary<IR::Neg>(pattern)); }
    Pattern operator~() const { return Pattern(new Unary<IR::Cmpl>(pattern)); }
    Pattern operator!() const { return Pattern(new Unary<IR::LNot>(pattern)); }
    Pattern operator*(const Pattern &r) const {
        return Pattern(new Binary<IR::Mul>(pattern, r.pattern, true));
    }
    Pattern operator/(const Pattern &r) const {
        return Pattern(new Binary<IR::Div>(pattern, r.pattern));
    }
    Pattern operator%(const Pattern &r) const {
        return Pattern(new Binary<IR::Mod>(pattern, r.pattern));
    }
    Pattern operator+(const Pattern &r) const {
        return Pattern(new Binary<IR::Add>(pattern, r.pattern, true));
    }
    Pattern operator-(const Pattern &r) const {
        return Pattern(new Binary<IR::Sub>(pattern, r.pattern));
    }
    Pattern operator<<(const Pattern &r) const {
        return Pattern(new Binary<IR::Shl>(pattern, r.pattern));
    }
    Pattern operator>>(const Pattern &r) const {
        return Pattern(new Binary<IR::Shr>(pattern, r.pattern));
    }
    Pattern operator==(const Pattern &r) const {
        return Pattern(new Binary<IR::Equ>(pattern, r.pattern, true));
    }
    Pattern operator!=(const Pattern &r) const {
        return Pattern(new Binary<IR::Neq>(pattern, r.pattern, true));
    }
    Pattern operator<(const Pattern &r) const {
        return Pattern(new Binary<IR::Lss>(pattern, r.pattern));
    }
    Pattern operator<=(const Pattern &r) const {
        return Pattern(new Binary<IR::Leq>(pattern, r.pattern));
    }
    Pattern operator>(const Pattern &r) const {
        return Pattern(new Binary<IR::Grt>(pattern, r.pattern));
    }
    Pattern operator>=(const Pattern &r) const {
        return Pattern(new Binary<IR::Geq>(pattern, r.pattern));
    }
    Pattern Relation(const Pattern &r) const {
        return Pattern(new Binary<IR::Operation::Relation>(pattern, r.pattern, true));
    }
    Pattern operator&(const Pattern &r) const {
        return Pattern(new Binary<IR::BAnd>(pattern, r.pattern, true));
    }
    Pattern operator|(const Pattern &r) const {
        return Pattern(new Binary<IR::BOr>(pattern, r.pattern, true));
    }
    Pattern operator^(const Pattern &r) const {
        return Pattern(new Binary<IR::BXor>(pattern, r.pattern, true));
    }
    Pattern operator&&(const Pattern &r) const {
        return Pattern(new Binary<IR::LAnd>(pattern, r.pattern));
    }
    Pattern operator||(const Pattern &r) const {
        return Pattern(new Binary<IR::LOr>(pattern, r.pattern));
    }
    /// We use these templates to deal with amgigous overloads introduced by C++20.
    /// https://en.cppreference.com/w/cpp/language/default_comparisons
    // TODO: Ideally, we would fix these ambiguous overloads by making the Pattern class explicit in
    // its initialization but that is a breaking change.
    template <class T>
    Pattern operator==(Match<T> &m) const {
        return *this == Pattern(m);
    }
    template <class T>
    Pattern operator!=(Match<T> &m) const {
        return *this != Pattern(m);
    }
    template <class T>
    Pattern operator==(const Match<T> &m) const {
        return *this == Pattern(m);
    }

    bool match(const IR::Node *n) { return pattern->match(n); }
};

inline Pattern operator*(int v, const Pattern &a) { return Pattern(v) * a; }
inline Pattern operator/(int v, const Pattern &a) { return Pattern(v) / a; }
inline Pattern operator%(int v, const Pattern &a) { return Pattern(v) % a; }
inline Pattern operator+(int v, const Pattern &a) { return Pattern(v) + a; }
inline Pattern operator-(int v, const Pattern &a) { return Pattern(v) - a; }
inline Pattern operator<<(int v, const Pattern &a) { return Pattern(v) << a; }
inline Pattern operator>>(int v, const Pattern &a) { return Pattern(v) >> a; }
inline Pattern operator==(int v, const Pattern &a) { return Pattern(v) == a; }
inline Pattern operator!=(int v, const Pattern &a) { return Pattern(v) != a; }
inline Pattern operator<(int v, const Pattern &a) { return Pattern(v) < a; }
inline Pattern operator<=(int v, const Pattern &a) { return Pattern(v) <= a; }
inline Pattern operator>(int v, const Pattern &a) { return Pattern(v) > a; }
inline Pattern operator>=(int v, const Pattern &a) { return Pattern(v) >= a; }
inline Pattern operator&(int v, const Pattern &a) { return Pattern(v) & a; }
inline Pattern operator|(int v, const Pattern &a) { return Pattern(v) | a; }
inline Pattern operator^(int v, const Pattern &a) { return Pattern(v) ^ a; }
inline Pattern operator&&(int v, const Pattern &a) { return Pattern(v) && a; }
inline Pattern operator||(int v, const Pattern &a) { return Pattern(v) || a; }

template <typename Node, typename Pattern>
bool match(const Node *n, const Pattern &P) {
    return const_cast<Pattern &>(P).match(n);
}

namespace detail {

template <typename Node>
struct Node_match {
    template <typename ITy>
    bool match(const ITy *n) {
        return n->template is<Node>();
    }
};

template <typename Type>
struct Type_match {
    template <typename ITy>
    bool match(const ITy *n) {
        if (const auto *e = n->template to<IR::Expression>()) {
            return e->type->template is<Type>();
        }
        return false;
    }
};

/// Inverting matcher
template <typename M>
struct Unless_match {
    M matcher;

    Unless_match(const M &matcher) : matcher(matcher) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(ITy *n) {
        return !matcher.match(n);
    }
};

/// Matching combinators
template <typename LM, typename RM>
struct AnyOf_match {
    LM lm;
    RM rm;

    AnyOf_match(const LM &left, const RM &right) : lm(left), rm(right) {}

    template <typename ITy>
    bool match(ITy *n) {
        if (lm.match(n)) return true;
        if (rm.match(n)) return true;
        return false;
    }
};

template <typename LM, typename RM>
struct AllOf_match {
    LM lm;
    RM rm;

    AllOf_match(const LM &left, const RM &right) : lm(left), rm(right) {}

    template <typename ITy>
    bool match(const ITy *n) {
        if (lm.match(n))
            if (rm.match(n)) return true;
        return false;
    }
};

struct BigInt_match {
    const big_int *&res;

    BigInt_match(const big_int *&res) : res(res) {}  // NOLINT(runtime/explicit)

    template <typename Node>
    bool match(const Node *n) {
        if (const auto *c = n->template to<IR::Constant>()) {
            res = &c->value;
            return true;
        }
        return false;
    }
};

template <int64_t val>
struct ConstantInt_match {
    template <typename ITy>
    bool match(const ITy *n) {
        if (const auto *c = n->template to<IR::Constant>()) {
            const big_int &ci = c->value;

            if (val >= 0) return ci == static_cast<uint64_t>(val);

            // If val is negative, and ci is shorter than it, truncate to the
            // right number of bits.  If it is larger, then we have to sign
            // extend.  Just compare their negated values.
            return -ci == -val;
        }
        return false;
    }
};

template <typename Node>
struct BindNode {
    const Node *&nodeRef;

    BindNode(const Node *&n) : nodeRef(n) {  // NOLINT(runtime/explicit)
        nodeRef = nullptr;
    }

    template <typename ITy>
    bool match(const ITy *n) {
        if (auto *cn = n->template to<Node>()) {
            nodeRef = cn;
            return true;
        }
        return false;
    }
};

template <typename Type>
struct BindType {
    const Type *&typeRef;

    BindType(const Type *&t) : typeRef(t) {  // NOLINT(runtime/explicit)
        typeRef = nullptr;
    }

    template <typename ITy>
    bool match(const ITy *n) {
        if (auto *e = n->template to<IR::Expression>()) {
            if (auto *ct = e->type->template to<Type>()) {
                typeRef = ct;
                return true;
            }
        }

        return false;
    }
};

struct SpecificNode_match {
    const IR::Node *node;

    SpecificNode_match(const IR::Node *n) : node(n) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(ITy *n) {
        return node == n;
    }
};

struct SpecificNodeEq_match {
    const IR::Node *node;

    SpecificNodeEq_match(const IR::Node *n) : node(n) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(ITy *n) {
        return node->equiv(*n);
    }
};

template <typename Node>
struct DeferredNode_match {
    Node *const &node;

    DeferredNode_match(Node *const &node) : node(node) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(ITy *const n) {
        BUG_CHECK(node, "expected matched node");
        return node == n;
    }
};

template <typename Node>
struct DeferredNodeEq_match {
    Node *const &node;

    DeferredNodeEq_match(Node *const &node) : node(node) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(ITy *const n) {
        BUG_CHECK(node, "expected matched node");
        return node->equiv(*n);
    }
};

// Matcher for specific unary operators.
template <typename OP, typename NodeType>
struct UnaryOp_match {
    OP x;

    UnaryOp_match(const OP &x) : x(x) {}  // NOLINT(runtime/explicit)

    template <typename OpTy>
    bool match(OpTy *n) {
        if (auto *uo = n->template to<NodeType>()) return x.match(uo->expr);
        return false;
    }
};

// Matcher for specific binary operators.
template <typename LHS, typename RHS, typename NodeType, bool Commutable = false>
struct BinaryOp_match {
    LHS l;
    RHS r;

    BinaryOp_match(const LHS &lhs, const RHS &rhs) : l(lhs), r(rhs) {}

    template <typename OpTy>
    bool match(OpTy *n) {
        if (auto *bo = n->template to<NodeType>()) {
            return (l.match(bo->left) && r.match(bo->right)) ||
                   (Commutable && l.match(bo->right) && l.match(bo->left));
        }
        return false;
    }
};

}  // namespace detail

/// Match an arbitrary node and ignore it.
inline auto m_Node() { return detail::Node_match<IR::Node>(); }

/// Match an arbitrary unary operation and ignore it.
inline auto m_UnOp() { return detail::Node_match<IR::Operation_Unary>(); }

/// Match an arbitrary binary operation and ignore it.
inline auto m_BinOp() { return detail::Node_match<IR::Operation_Binary>(); }

/// Match an arbitrary relation operation and ignore it.
inline auto m_RelOp() { return detail::Node_match<IR::Operation_Relation>(); }

/// Match an arbitrary Constant and ignore it.
inline auto m_Constant() { return detail::Node_match<IR::Constant>(); }

/// Match an arbitrary Expression and ignore it.
inline auto m_Expr() { return detail::Node_match<IR::Expression>(); }

/// Match an arbitrary Type and ignore it.
inline auto m_Type() { return detail::Node_match<IR::Type>(); }

/// Match an arbitrary PathExpression and ignore it.
inline auto m_PathExpr() { return detail::Node_match<IR::PathExpression>(); }

/// Match a specific Type of a node (expression) and ignore it.
template <class Type>
inline auto m_Type() {
    return detail::Type_match<Type>();
}
/// Match IR::Type_InfInt
inline auto m_TypeInfInt() { return m_Type<IR::Type_InfInt>(); }
/// Match IR::Type_Bits
inline auto m_TypeBits() { return m_Type<IR::Type_Bits>(); }

/// Match if the inner matcher does *NOT* match.
template <typename Matcher>
inline auto m_Unless(const Matcher &m) {
    return detail::Unless_match<Matcher>(m);
}

/// Combine two pattern matchers matching L || R
template <typename LM, typename RM>
inline auto m_AnyOf(const LM &l, const RM &r) {
    return detail::AnyOf_match<LM, RM>(l, r);
}

/// Combine two pattern matchers matching L && R
template <typename LM, typename RM>
inline auto m_AllOf(const LM &l, const RM &r) {
    return detail::AllOf_match<LM, RM>(l, r);
}

/// Match a Constant, binding the specified pointer to the contained big_int.
inline auto m_BigInt(const big_int *&res) { return detail::BigInt_match(res); }

/// Match a Constant with int of a specific value.
template <int64_t val>
inline auto m_ConstantInt() {
    return detail::ConstantInt_match<val>();
}

/// Match a node, capturing it if we match.
inline detail::BindNode<const IR::Node> m_Node(const IR::Node *&n) { return n; }
/// Match a Constant, capturing the value if we match.
inline detail::BindNode<IR::Constant> m_Constant(const IR::Constant *&c) { return c; }
/// Match an Expression, capturing the value if we match.
inline detail::BindNode<IR::Expression> m_Expr(const IR::Expression *&e) { return e; }
/// Match a PathExpression, capturing the value if we match.
inline detail::BindNode<IR::PathExpression> m_PathExpr(const IR::PathExpression *&e) { return e; }
/// Match if we have a specific specified node. Uses `operator==` for checking node equivalence
inline detail::SpecificNode_match m_Specific(const IR::Node *n) { return n; }
// Same as m_Specific, but calls `equiv` for deep comparison
inline detail::SpecificNodeEq_match m_SpecificEq(const IR::Node *n) { return n; }
/// Like m_Specific(), but works if the specific value to match is determined as
/// part of the same match() expression.
/// For example: m_Add(m_Node(X), m_Specific(X)) is incorrect, because m_Specific()
/// will bind X before the pattern match starts.  m_Add(m_Node(X), m_Deferred(X)) is correct,
/// and will check against whichever value m_Node(X) populated.
template <class Node>
inline detail::DeferredNode_match<Node> m_Deferred(Node *const &n) {
    return n;
}
template <class Node>
inline detail::DeferredNode_match<Node> m_Deferred(const Node *const &n) {
    return n;
}
// Same as m_Deferred, but calls `equiv` for deep comparison
template <class Node>
inline detail::DeferredNodeEq_match<IR::Node> m_DeferredEq(Node *const &n) {
    return n;
}
template <class Node>
inline detail::DeferredNodeEq_match<const Node> m_DeferredEq(const Node *const &n) {
    return n;
}

/// Match a unary operator, capturing it if we match.
inline detail::BindNode<IR::Operation_Unary> m_UnOp(const IR::Operation_Unary *&uo) { return uo; }
/// Match a binary operator, capturing it if we match.
inline detail::BindNode<IR::Operation_Binary> m_BinOp(const IR::Operation_Binary *&bo) {
    return bo;
}
/// Match a relation operator, capturing it if we match.
inline detail::BindNode<IR::Operation_Relation> m_RelOp(const IR::Operation_Relation *&ro) {
    return ro;
}
/// Match an Cmpl, capturing the value if we match.
inline detail::BindNode<IR::Cmpl> m_Cmpl(const IR::Cmpl *&c) { return c; }

/// Match a type, capturing it if we match.
template <typename Type>
inline auto m_Type(Type *&t) {
    return detail::BindType<Type>(t);
}

template <typename Type>
inline auto m_Type(const Type *&t) {
    return detail::BindType<const Type>(t);
}
/// Match IR::Type_Bits, capturing it if we match
inline auto m_TypeBits(const IR::Type_Bits *&t) { return m_Type<IR::Type_Bits>(t); }

template <typename LHS, typename RHS>
inline auto m_BinOp(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::Operation_Binary>(l, r);
}

template <typename LHS, typename RHS>
inline auto m_RelOp(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::Operation_Relation>(l, r);
}

template <typename OP, typename NodeType = IR::Operation_Unary>
inline auto m_UnOp(const OP &x) {
    return detail::UnaryOp_match<OP, NodeType>(x);
}

template <typename LHS, typename RHS>
inline auto m_Add(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::Add>(l, r);
}

template <typename LHS, typename RHS>
inline auto m_BAnd(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::BAnd>(l, r);
}

template <typename LHS, typename RHS>
inline auto m_BOr(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::BOr>(l, r);
}

template <typename LHS, typename RHS>
inline auto m_Mul(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::Mul>(l, r);
}

template <typename LHS, typename RHS>
inline auto m_Shl(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::Shl>(l, r);
}

template <typename LHS, typename RHS>
inline auto m_Shr(const LHS &l, const RHS &r) {
    return detail::BinaryOp_match<LHS, RHS, IR::Shr>(l, r);
}

}  // namespace P4

#endif /* IR_PATTERN_H_ */
