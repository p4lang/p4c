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
struct node_match {
    template <typename ITy>
    bool match(const ITy *n) {
        return n->template is<Node>();
    }
};

/// Inverting matcher
template <typename M>
struct match_unless {
    M matcher;

    match_unless(const M &matcher) : matcher(matcher) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(ITy *n) {
        return !matcher.match(n);
    }
};

/// Matching combinators
template <typename LM, typename RM>
struct match_combine_or {
    LM lm;
    RM rm;

    match_combine_or(const LM &left, const RM &right) : lm(left), rm(right) {}

    template <typename ITy>
    bool match(ITy *n) {
        if (lm.match(n)) return true;
        if (rm.match(n)) return true;
        return false;
    }
};

template <typename LM, typename RM>
struct match_combine_and {
    LM lm;
    RM rm;

    match_combine_and(const LM &left, const RM &right) : lm(left), rm(right) {}

    template <typename ITy>
    bool match(const ITy *n) {
        if (lm.match(n))
            if (rm.match(n)) return true;
        return false;
    }
};

struct bigint_match {
    const big_int *&res;

    bigint_match(const big_int *&res) : res(res) {}  // NOLINT(runtime/explicit)

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
struct constantint_match {
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
struct bind_node {
    const Node *&nr;

    bind_node(const Node *&n) : nr(n) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(const ITy *n) {
        if (auto *cn = n->template to<Node>()) {
            nr = cn;
            return true;
        }
        return false;
    }
};

struct specificnode_ty {
    const IR::Node *node;

    specificnode_ty(const IR::Node *n) : node(n) {}  // NOLINT(runtime/explicit)

    template <typename ITy>
    bool match(ITy *n) {
        return node == n;
    }
};

template <typename LHS, typename RHS, bool Commutable = false>
struct AnyBinaryOp_match {
    LHS l;
    RHS r;

    AnyBinaryOp_match(const LHS &l, const RHS &r) : l(l), r(r) {}

    template <typename OpTy>
    bool match(OpTy *n) {
        if (auto *bo = n->template to<IR::Operation_Binary>())
            return (l.match(bo->left) && r.match(bo->right)) ||
                   (Commutable && l.match(bo->right) && r.match(bo->left));

        return false;
    }
};

template <typename OP>
struct AnyUnaryOp_match {
    OP x;

    AnyUnaryOp_match(const OP &x) : x(x) {}  // NOLINT(runtime/explicit)

    template <typename OpTy>
    bool match(OpTy *n) {
        if (auto *uo = n->template to<IR::Operation_Unary>()) return x.match(uo->expr);
        return false;
    }
};

}  // namespace detail

/// Match an arbitrary node and ignore it.
inline auto m_Node() { return detail::node_match<IR::Node>(); }

/// Match an arbitrary unary operation and ignore it.
inline auto m_UnOp() { return detail::node_match<IR::Operation_Unary>(); }

/// Match an arbitrary binary operation and ignore it.
inline auto m_BinOp() { return detail::node_match<IR::Operation_Binary>(); }

/// Match an arbitrary relation operation and ignore it.
inline auto m_RelOp() { return detail::node_match<IR::Operation_Relation>(); }

/// Match an arbitrary Constant and ignore it.
inline auto m_Constant() { return detail::node_match<IR::Constant>(); }

/// Match an arbitrary Expression and ignore it.
inline auto m_Expr() { return detail::node_match<IR::Expression>(); }

/// Match if the inner matcher does *NOT* match.
template <typename Matcher>
inline auto m_Unless(const Matcher &m) {
    return detail::match_unless<Matcher>(m);
}

/// Combine two pattern matchers matching L || R
template <typename LM, typename RM>
inline auto m_CombineOr(const LM &l, const RM &r) {
    return detail::match_combine_or<LM, RM>(l, r);
}

/// Combine two pattern matchers matching L && R
template <typename LM, typename RM>
inline auto m_CombineAnd(const LM &l, const RM &r) {
    return detail::match_combine_and<LM, RM>(l, r);
}

/// Match a Constant, binding the specified pointer to the contained big_int.
inline auto m_BigInt(const big_int *&res) { return detail::bigint_match(res); }

/// Match a Constant with int of a specific value.
template <int64_t val>
inline auto m_ConstantInt() {
    return detail::constantint_match<val>();
}

/// Match a node, capturing it if we match.
inline detail::bind_node<const IR::Node> m_Node(const IR::Node *&n) { return n; }
/// Match a Constant, capturing the value if we match.
inline detail::bind_node<IR::Constant> m_Constant(const IR::Constant *&c) { return c; }
/// Match an Expression, capturing the value if we match.
inline detail::bind_node<IR::Expression> m_Expr(const IR::Expression *&e) { return e; }
/// Match if we have a specific specified node.
inline detail::specificnode_ty m_Specific(const IR::Node *n) { return n; }

/// Match a unary operator, capturing it if we match.
inline detail::bind_node<IR::Operation_Unary> m_UnOp(const IR::Operation_Unary *&uo) { return uo; }
/// Match a binary operator, capturing it if we match.
inline detail::bind_node<IR::Operation_Binary> m_BinOp(const IR::Operation_Binary *&bo) {
    return bo;
}
/// Match a relation operator, capturing it if we match.
inline detail::bind_node<IR::Operation_Relation> m_RelOp(const IR::Operation_Relation *&ro) {
    return ro;
}

template <typename LHS, typename RHS>
inline auto m_BinOp(const LHS &l, const RHS &r) {
    return detail::AnyBinaryOp_match<LHS, RHS>(l, r);
}

template <typename OP>
inline auto m_UnOp(const OP &x) {
    return detail::AnyUnaryOp_match<OP>(x);
}
}  // namespace P4

#endif /* IR_PATTERN_H_ */
