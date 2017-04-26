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

bool StrengthReduction::isOne(const IR::Expression* expr) const {
    auto cst = expr->to<IR::Constant>();
    if (cst == nullptr)
        return false;
    return cst->value == 1;
}

bool StrengthReduction::isZero(const IR::Expression* expr) const {
    auto cst = expr->to<IR::Constant>();
    if (cst == nullptr)
        return false;
    return cst->value == 0;
}

bool StrengthReduction::isTrue(const IR::Expression* expr) const {
    auto cst = expr->to<IR::BoolLiteral>();
    if (cst == nullptr)
        return false;
    return cst->value;
}

bool StrengthReduction::isFalse(const IR::Expression* expr) const {
    auto cst = expr->to<IR::BoolLiteral>();
    if (cst == nullptr)
        return false;
    return !cst->value;
}
int StrengthReduction::isPowerOf2(const IR::Expression* expr) const {
    auto cst = expr->to<IR::Constant>();
    if (cst == nullptr)
        return -1;
    mpz_class value = cst->value;
    if (sgn(value) <= 0)
        return -1;
    auto bitcnt = mpz_popcount(value.get_mpz_t());
    if (bitcnt != 1)
        return -1;
    auto log = mpz_scan1(value.get_mpz_t(), 0);
    // Assumes value does not have more than 2 billion bits
    return log;
}

/// @section Visitor Methods

const IR::Node* StrengthReduction::postorder(IR::BAnd* expr) {
    if (isZero(expr->left))
        return expr->left;
    if (isZero(expr->right))
        return expr->right;
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::BOr* expr) {
    if (isZero(expr->left))
        return expr->right;
    if (isZero(expr->right))
        return expr->left;
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::BXor* expr) {
    if (isZero(expr->left))
        return expr->right;
    if (isZero(expr->right))
        return expr->left;
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::LAnd* expr) {
    if (isFalse(expr->left))
        return expr->left;
    if (isTrue(expr->left))
        return expr->right;
    if (isTrue(expr->right))
        return expr->left;
    // Note that remaining case is not simplified, due to possible side effects in expr->left
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::LOr* expr) {
    if (isFalse(expr->left))
        return expr->right;
    if (isTrue(expr->left))
        return expr->left;
    if (isFalse(expr->right))
        return expr->left;
    // Note that remaining case is not simplified, due to semantics of short-circuit evaluation
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::Sub* expr) {
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

const IR::Node* StrengthReduction::postorder(IR::Add* expr) {
    if (isZero(expr->right))
        return expr->left;
    if (isZero(expr->left))
        return expr->right;
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::Shl* expr) {
    if (isZero(expr->right) || isZero(expr->left))
        return expr->left;
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::Shr* expr) {
    if (isZero(expr->right) || isZero(expr->left))
        return expr->left;
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::Mul* expr) {
    if (isZero(expr->left))
        return expr->left;
    if (isZero(expr->right))
        return expr->right;
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
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::Div* expr) {
    if (isZero(expr->right)) {
        ::error("%1%: Division by zero", expr);
        return expr;
    }
    if (isZero(expr->left))
        return expr->left;
    if (isOne(expr->right))
        return expr->left;
    auto exp = isPowerOf2(expr->right);
    if (exp >= 0) {
        auto amt = new IR::Constant(exp);
        auto sh = new IR::Shr(expr->srcInfo, expr->left, amt);
        return sh;
    }
    return expr;
}

const IR::Node* StrengthReduction::postorder(IR::Mod* expr) {
    if (isZero(expr->right)) {
        ::error("%1%: Modulo by zero", expr);
        return expr;
    }
    if (isZero(expr->left))
        return expr->left;
    auto exp = isPowerOf2(expr->right);
    if (exp >= 0) {
        mpz_class mask = 1;
        mask = (mask << exp) - 1;
        auto amt = new IR::Constant(expr->right->to<IR::Constant>()->type, mask);
        auto sh = new IR::BAnd(expr->srcInfo, expr->left, amt);
        return sh;
    }
    return expr;
}

}  // namespace P4
