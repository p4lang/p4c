/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "strengthReduction.h"

namespace P4 {

/// @section Helper methods

bool DoStrengthReduction::isOne(const IR::Expression* expr) const {
    auto cst = expr->to<IR::Constant>();
    if (cst == nullptr)
        return false;
    return cst->value == 1;
}

bool DoStrengthReduction::isZero(const IR::Expression* expr) const {
    auto cst = expr->to<IR::Constant>();
    if (cst == nullptr)
        return false;
    return cst->value == 0;
}

bool DoStrengthReduction::isTrue(const IR::Expression* expr) const {
    auto cst = expr->to<IR::BoolLiteral>();
    if (cst == nullptr)
        return false;
    return cst->value;
}

bool DoStrengthReduction::isFalse(const IR::Expression* expr) const {
    auto cst = expr->to<IR::BoolLiteral>();
    if (cst == nullptr)
        return false;
    return !cst->value;
}
int DoStrengthReduction::isPowerOf2(const IR::Expression* expr) const {
    auto cst = expr->to<IR::Constant>();
    if (cst == nullptr)
        return -1;
    if (cst->value <= 0)
        return -1;
    auto log = boost::multiprecision::msb(cst->value);
    if (log != boost::multiprecision::lsb(cst->value))
        return -1;
    // Assumes value does not have more than 2 billion bits
    return log;
}

bool DoStrengthReduction::isAllOnes(const IR::Expression* expr) const {
    auto cst = expr->to<IR::Constant>();
    if (cst == nullptr)
        return false;
    big_int value = cst->value;
    if (value <= 0)
        return false;
    auto bitcnt = bitcount(value);
    return bitcnt == (unsigned long)(expr->type->width_bits());
}

/// @section Visitor Methods

const IR::Node* DoStrengthReduction::postorder(IR::Cmpl* expr) {
    if (auto a = expr->expr->to<IR::Cmpl>())
        return a->expr;
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::BAnd* expr) {
    if (isAllOnes(expr->left))
        return expr->right;
    if (isAllOnes(expr->right))
        return expr->left;
    auto l = expr->left->to<IR::Cmpl>();
    auto r = expr->right->to<IR::Cmpl>();
    if (l && r)
        return new IR::Cmpl(new IR::BOr(expr->srcInfo, l->expr, r->expr));

    if (hasSideEffects(expr))
        return expr;
    if (isZero(expr->left))
        return expr->left;
    if (isZero(expr->right))
        return expr->right;
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::BOr* expr) {
    if (isZero(expr->left))
        return expr->right;
    if (isZero(expr->right))
        return expr->left;
    auto l = expr->left->to<IR::Cmpl>();
    auto r = expr->right->to<IR::Cmpl>();
    if (l && r)
        return new IR::Cmpl(new IR::BAnd(expr->srcInfo, l->expr, r->expr));
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::BXor* expr) {
    if (isZero(expr->left))
        return expr->right;
    if (isZero(expr->right))
        return expr->left;
    bool cmpl = false;
    if (auto l = expr->left->to<IR::Cmpl>()) {
        expr->left = l->expr;
        cmpl = !cmpl; }
    if (auto r = expr->right->to<IR::Cmpl>()) {
        expr->right = r->expr;
        cmpl = !cmpl; }
    if (cmpl)
        return new IR::Cmpl(expr);
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::LAnd* expr) {
    if (isFalse(expr->left))
        return expr->left;
    if (isTrue(expr->left))
        return expr->right;
    if (isTrue(expr->right))
        return expr->left;
    // Note that remaining case is not simplified, due to possible side effects in expr->left
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::LOr* expr) {
    if (isFalse(expr->left))
        return expr->right;
    if (isTrue(expr->left))
        return expr->left;
    if (isFalse(expr->right))
        return expr->left;
    // Note that remaining case is not simplified, due to semantics of short-circuit evaluation
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::LNot* expr) {
    if (auto e = expr->expr->to<IR::LNot>())
        return e->expr;
    if (auto e = expr->expr->to<IR::Equ>())
        return new IR::Neq(e->left, e->right);
    if (auto e = expr->expr->to<IR::Neq>())
        return new IR::Equ(e->left, e->right);
    if (auto e = expr->expr->to<IR::Leq>())
        return new IR::Grt(e->left, e->right);
    if (auto e = expr->expr->to<IR::Geq>())
        return new IR::Lss(e->left, e->right);
    if (auto e = expr->expr->to<IR::Lss>())
        return new IR::Geq(e->left, e->right);
    if (auto e = expr->expr->to<IR::Grt>())
        return new IR::Leq(e->left, e->right);
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Sub* expr) {
    if (isZero(expr->right))
        return expr->left;
    if (isZero(expr->left))
        return new IR::Neg(expr->srcInfo, expr->right);
    // Replace `a - constant` with `a + (-constant)`
    if (expr->right->is<IR::Constant>()) {
        auto cst = expr->right->to<IR::Constant>();
        auto neg = new IR::Constant(cst->srcInfo, cst->type, -cst->value, cst->base, true);
        auto result = new IR::Add(expr->srcInfo, expr->left, neg);
        return result;
    }
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Add* expr) {
    if (isZero(expr->right))
        return expr->left;
    if (isZero(expr->left))
        return expr->right;
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Shl* expr) {
    if (isZero(expr->right))
        return expr->left;
    if (auto sh2 = expr->left->to<IR::Shl>()) {
        if (sh2->right->type->is<IR::Type_InfInt>() &&
            expr->right->type->is<IR::Type_InfInt>()) {
            // (a << b) << c is a << (b + c)
            auto result = new IR::Shl(expr->srcInfo, sh2->left,
                                      new IR::Add(expr->srcInfo, sh2->right, expr->right));
            LOG3("Replace " << expr << " with " << result);
            return result;
        }
    }

    if (!hasSideEffects(expr->right) && isZero(expr->left))
        return expr->left;
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Shr* expr) {
    if (isZero(expr->right))
        return expr->left;
    if (auto sh2 = expr->left->to<IR::Shr>()) {
        if (sh2->right->type->is<IR::Type_InfInt>() &&
            expr->right->type->is<IR::Type_InfInt>()) {
            // (a >> b) >> c is a >> (b + c)
            auto result = new IR::Shr(expr->srcInfo, sh2->left,
                                      new IR::Add(expr->srcInfo, sh2->right, expr->right));
            LOG3("Replace " << expr << " with " << result);
            return result;
        }
    }
    if (!hasSideEffects(expr->right) && isZero(expr->left))
        return expr->left;
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Mul* expr) {
    if (isOne(expr->left))
        return expr->right;
    if (isOne(expr->right))
        return expr->left;
    auto exp = isPowerOf2(expr->left);
    if (exp >= 0) {
        auto amt = new IR::Constant(exp);
        auto sh = new IR::Shl(expr->srcInfo, expr->right, amt);
        return sh;
    }
    exp = isPowerOf2(expr->right);
    if (exp >= 0) {
        auto amt = new IR::Constant(exp);
        auto sh = new IR::Shl(expr->srcInfo, expr->left, amt);
        return sh;
    }
    if (hasSideEffects(expr))
        return expr;
    if (isZero(expr->left))
        return expr->left;
    if (isZero(expr->right))
        return expr->right;
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Div* expr) {
    if (isZero(expr->right)) {
        ::error("%1%: Division by zero", expr);
        return expr;
    }
    if (isOne(expr->right))
        return expr->left;
    auto exp = isPowerOf2(expr->right);
    if (exp >= 0) {
        auto amt = new IR::Constant(exp);
        auto sh = new IR::Shr(expr->srcInfo, expr->left, amt);
        return sh;
    }
    if (isZero(expr->left) && !hasSideEffects(expr->right))
        return expr->left;
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Mod* expr) {
    if (isZero(expr->right)) {
        ::error("%1%: Modulo by zero", expr);
        return expr;
    }
    if (isZero(expr->left) && !hasSideEffects(expr->right))
        return expr->left;
    auto exp = isPowerOf2(expr->right);
    if (exp >= 0) {
        big_int mask = 1;
        mask = (mask << exp) - 1;
        auto amt = new IR::Constant(expr->right->to<IR::Constant>()->type, mask);
        auto sh = new IR::BAnd(expr->srcInfo, expr->left, amt);
        return sh;
    }
    return expr;
}

const IR::Node* DoStrengthReduction::postorder(IR::Slice* expr) {
    int shift_amt = 0;
    const IR::Expression *shift_of = nullptr;
    if (auto sh = expr->e0->to<IR::Shr>()) {
        if (auto k = sh->right->to<IR::Constant>()) {
            shift_amt = k->asInt();
            shift_of = sh->left; } }
    if (auto sh = expr->e0->to<IR::Shl>()) {
        if (auto k = sh->right->to<IR::Constant>()) {
            shift_amt = -k->asInt();
            shift_of = sh->left; } }
    if (shift_of) {
        if (!shift_of->type->is<IR::Type_Bits>())
            return expr;
        if (shift_of->type->to<IR::Type_Bits>()->isSigned)
            return expr;
        int hi = expr->getH();
        int lo = expr->getL();
        if (lo + shift_amt >= 0 && hi + shift_amt < shift_of->type->width_bits()) {
            expr->e0 = shift_of;
            expr->e1 = new IR::Constant(hi + shift_amt);
            expr->e2 = new IR::Constant(lo + shift_amt); }
        if (hi + shift_amt <= 0) {
            if (!hasSideEffects(shift_of))
                return new IR::Constant(IR::Type_Bits::get(hi - lo + 1), 0);
            // TODO: here we could promote the side-effect into a
            // separate statement.  and still return the constant.
            // But for now we only produce expressions.
            return expr;
        }
        if (lo + shift_amt < 0) {
            expr->e0 = shift_of;
            expr->e1 = new IR::Constant(hi + shift_amt);
            expr->e2 = new IR::Constant(0);
            return new IR::Concat(expr->type, expr,
                    new IR::Constant(IR::Type_Bits::get(-(lo + shift_amt)), 0)); } }

    while (auto cat = expr->e0->to<IR::Concat>()) {
        unsigned rwidth = cat->right->type->width_bits();
        if (expr->getL() >= rwidth) {
            if (!hasSideEffects(cat->right)) {
                expr->e0 = cat->left;
                expr->e1 = new IR::Constant(expr->getH() - rwidth);
                expr->e2 = new IR::Constant(expr->getL() - rwidth);
            } else {
                break;
            }
        } else if (expr->getH() < rwidth) {
            if (!hasSideEffects(cat->left))
                expr->e0 = cat->right;
            else
                break;
        } else {
            return new IR::Concat(expr->type,
                    new IR::Slice(cat->left, expr->getH() - rwidth, 0),
                    new IR::Slice(cat->right, rwidth-1, expr->getL()));
        }
    }

    while (auto cast = expr->e0->to<IR::Cast>()) {
        if (expr->getH() < size_t(cast->expr->type->width_bits())) {
            expr->e0 = cast->expr;
        } else {
            break; } }

    // out-of-bound error has been caught in type checking
    if (auto sl = expr->e0->to<IR::Slice>()) {
        auto e = sl->e0;
        auto hi = expr->getH() + sl->getL();
        auto lo = expr->getL() + sl->getL();
        return new IR::Slice(e, hi, lo);
    }

    auto slice_width = expr->getH() - expr->getL() + 1;
    if (slice_width == (unsigned)expr->e0->type->width_bits() &&
        !hasSideEffects(expr->e1))
        return expr->e0;

    return expr;
}

}  // namespace P4
