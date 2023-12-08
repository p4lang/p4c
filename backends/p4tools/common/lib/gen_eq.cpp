#include "backends/p4tools/common/lib/gen_eq.h"

#include <cstddef>

#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/exceptions.h"

namespace P4Tools::GenEq {

/// Recursively resolve lists of size 1 by returning the expression contained within.
const IR::Expression *resolveSingletonList(const IR::Expression *expr) {
    if (const auto *listExpr = expr->to<IR::BaseListExpression>()) {
        if (listExpr->size() == 1) {
            return resolveSingletonList(listExpr->components.at(0));
        }
    }
    if (const auto *structExpr = expr->to<IR::StructExpression>()) {
        if (structExpr->size() == 1) {
            return resolveSingletonList(structExpr->components.at(0)->expression);
        }
    }
    return expr;
}

/// Flatten and compare two lists.
/// Members of struct expressions are ordered and compared based on the underlying type.
const IR::Expression *equateListTypes(const IR::Expression *left, const IR::Expression *right) {
    std::vector<const IR::Expression *> leftElems = IR::flattenListOrStructExpression(left);
    std::vector<const IR::Expression *> rightElems = IR::flattenListOrStructExpression(right);

    auto leftElemsSize = leftElems.size();
    auto rightElemsSize = rightElems.size();
    BUG_CHECK(leftElemsSize == rightElemsSize,
              "The size of left elements (%1%) and the size of right elements (%2%) are "
              "different.",
              leftElemsSize, rightElemsSize);

    const IR::Expression *result = new IR::BoolLiteral(IR::Type::Boolean::get(), true);
    bool firstLoop = true;
    for (size_t i = 0; i < leftElems.size(); i++) {
        const auto *conjunct = equate(leftElems.at(i), rightElems.at(i));
        if (firstLoop) {
            result = conjunct;
            firstLoop = false;
        } else {
            result = new IR::LAnd(IR::Type::Boolean::get(), result, conjunct);
        }
    }
    return result;
}

/// Construct an equality expression.
const IR::Equ *mkEq(const IR::Expression *e1, const IR::Expression *e2) {
    return new IR::Equ(IR::Type::Boolean::get(), e1, e2);
}

const IR::Expression *equate(const IR::Expression *left, const IR::Expression *right) {
    // First, recursively unroll any singleton elements.
    left = resolveSingletonList(left);
    right = resolveSingletonList(right);

    // A single default expression can be matched with a list expression.
    if (left->is<IR::DefaultExpression>() || right->is<IR::DefaultExpression>()) {
        return new IR::BoolLiteral(IR::Type::Boolean::get(), true);
    }

    // If we still have lists after unrolling, compare them.
    if (left->is<IR::BaseListExpression>() || left->is<IR::StructExpression>()) {
        BUG_CHECK(right->is<IR::BaseListExpression>() || right->is<IR::StructExpression>(),
                  "Right expression must be a list-like expression. Is %1% of type %2%.", right,
                  right->node_type_name());
        return equateListTypes(left, right);
    }

    // At this point, all lists must be resolved.
    if (const auto *maskKey = right->to<IR::Mask>()) {
        // Let a &&& b represent the keyset.
        // We return a & b == target & b.
        return mkEq(new IR::BAnd(left->type, maskKey->left, maskKey->right),
                    new IR::BAnd(left->type, left, maskKey->right));
    }

    if (const auto *rangeKey = right->to<IR::Range>()) {
        const auto *boolType = IR::Type::Boolean::get();
        return new IR::LAnd(boolType, new IR::Leq(boolType, rangeKey->left, left),
                            new IR::Leq(boolType, left, rangeKey->right));
    }

    return mkEq(left, right);
}

}  // namespace P4Tools::GenEq
