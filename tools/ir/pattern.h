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
    Pattern(Base *p) : pattern(p) {}

    template<class T> class MatchExt : public Base {
        const T *&m;
     public:
        bool match(const IR::Node *n) override { return (m = n->to<T>()); }
        MatchExt(const T *&m) : m(m) {}
    };

    class Const : public Base {
        mpz_class       value;
     public:
        bool match(const IR::Node *n) override {
            if (auto k = n->to<IR::Constant>())
                return k->value == value;
            return false; }
        Const(mpz_class v) : value(v) {}
        Const(int v) : value(v) {}
    };
    template<class T> class Unary : public Base {
        Base *expr;
     public:
        bool match(const IR::Node *n) override {
            if (auto b = n->to<T>())
                return expr->match(b->expr);
            return false; }
        Unary(Base *e) : expr(e) {}
    };
    template<class T> class Binary : public Base {
        Base    *left, *right;
        bool    commutative;
     public:
        bool match(const IR::Node *n) override {
            if (auto b = n->to<T>()) {
                if (left->match(b->left) && right->match(b->right))
                    return true;
                if (commutative && left->match(b->right) && right->match(b->left))
                    return true; }
            return false; }
        Binary(Base *l, Base *r, bool commute = false) : left(l), right(r), commutative(commute) {}
    };

 public:

    template<class T> class Match : public Base {
        const T *m;
     public:
        bool match(const IR::Node *n) override { return (m = n->to<T>()); }
        Match() : m(nullptr) {}
        const T *operator->() const { return m; }
        operator const T *() const { return m; }
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
        Pattern operator&(const Pattern &a) { return Pattern(*this) & a; }
        Pattern operator|(const Pattern &a) { return Pattern(*this) | a; }
        Pattern operator^(const Pattern &a) { return Pattern(*this) ^ a; }
        Pattern operator&&(const Pattern &a) { return Pattern(*this) && a; }
        Pattern operator||(const Pattern &a) { return Pattern(*this) || a; }
    };

    template <class T> Pattern(const T*&m) : pattern(new MatchExt<T>(m)) {}
    template <class T>Pattern(Match<T> &m) : pattern(&m) {}
    Pattern(mpz_class v) : pattern(new Const(v)) {}     // NOLINT(runtime/explicit)
    Pattern(int v) : pattern(new Const(v)) {}           // NOLINT(runtime/explicit)
    Pattern operator-() const { return Pattern(new Unary<IR::Neg>(pattern)); }
    Pattern operator~() const { return Pattern(new Unary<IR::Cmpl>(pattern)); }
    Pattern operator!() const { return Pattern(new Unary<IR::LNot>(pattern)); }
    Pattern operator*(const Pattern &r) const {
        return Pattern(new Binary<IR::Mul>(pattern, r.pattern, true)); }
    Pattern operator/(const Pattern &r) const {
        return Pattern(new Binary<IR::Div>(pattern, r.pattern)); }
    Pattern operator%(const Pattern &r) const {
        return Pattern(new Binary<IR::Mod>(pattern, r.pattern)); }
    Pattern operator+(const Pattern &r) const {
        return Pattern(new Binary<IR::Add>(pattern, r.pattern, true)); }
    Pattern operator-(const Pattern &r) const {
        return Pattern(new Binary<IR::Sub>(pattern, r.pattern)); }
    Pattern operator<<(const Pattern &r) const {
        return Pattern(new Binary<IR::Shl>(pattern, r.pattern)); }
    Pattern operator>>(const Pattern &r) const {
        return Pattern(new Binary<IR::Shr>(pattern, r.pattern)); }
    Pattern operator==(const Pattern &r) const {
        return Pattern(new Binary<IR::Equ>(pattern, r.pattern, true)); }
    Pattern operator!=(const Pattern &r) const {
        return Pattern(new Binary<IR::Neq>(pattern, r.pattern, true)); }
    Pattern operator<(const Pattern &r) const {
        return Pattern(new Binary<IR::Lss>(pattern, r.pattern)); }
    Pattern operator<=(const Pattern &r) const {
        return Pattern(new Binary<IR::Leq>(pattern, r.pattern)); }
    Pattern operator>(const Pattern &r) const {
        return Pattern(new Binary<IR::Grt>(pattern, r.pattern)); }
    Pattern operator>=(const Pattern &r) const {
        return Pattern(new Binary<IR::Geq>(pattern, r.pattern)); }
    Pattern operator&(const Pattern &r) const {
        return Pattern(new Binary<IR::BAnd>(pattern, r.pattern, true)); }
    Pattern operator|(const Pattern &r) const {
        return Pattern(new Binary<IR::BOr>(pattern, r.pattern, true)); }
    Pattern operator^(const Pattern &r) const {
        return Pattern(new Binary<IR::BXor>(pattern, r.pattern, true)); }
    Pattern operator&&(const Pattern &r) const {
        return Pattern(new Binary<IR::LAnd>(pattern, r.pattern)); }
    Pattern operator||(const Pattern &r) const {
        return Pattern(new Binary<IR::LOr>(pattern, r.pattern)); }

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


#endif /* IR_PATTERN_H_ */
