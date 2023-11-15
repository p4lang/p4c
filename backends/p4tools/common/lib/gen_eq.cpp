#include "backends/p4tools/common/lib/gen_eq.h"

#include <cstddef>

#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/exceptions.h"

namespace P4Tools {

const IR::Expression *GenEq::checkSingleton(const IR::Expression *expr) {
    if (const auto *listExpr = expr->to<IR::BaseListExpression>()) {
        if (listExpr->size() == 1) {
            expr = checkSingleton(listExpr->components.at(0));
        }
    } else if (const auto *structExpr = expr->to<IR::StructExpression>()) {
        if (structExpr->size() == 1) {
            expr = checkSingleton(structExpr->components.at(0)->expression);
        }
    }
    return expr;
}

const IR::Expression *GenEq::equateListTypes(const IR::Expression *left,
                                             const IR::Expression *right) {
    std::vector<const IR::Expression *> leftElems;
    if (auto listExpr = left->to<IR::BaseListExpression>()) {
        leftElems = IR::flattenListExpression(listExpr);
    } else if (auto structExpr = left->to<IR::StructExpression>()) {
        leftElems = IR::flattenStructExpression(structExpr);
    } else {
        BUG("Unsupported list expression %1% of type %2%.", left, left->node_type_name());
    }

    std::vector<const IR::Expression *> rightElems;
    if (auto listExpr = right->to<IR::BaseListExpression>()) {
        rightElems = IR::flattenListExpression(listExpr);
    } else if (auto structExpr = right->to<IR::StructExpression>()) {
        rightElems = IR::flattenStructExpression(structExpr);
    } else {
        BUG("Unsupported right list expression %1% of type %2%.", right, right->node_type_name());
    }
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

const IR::Expression *GenEq::equate(const IR::Expression *left, const IR::Expression *right) {
    // First, recursively unroll any singleton elements.
    left = checkSingleton(left);
    right = checkSingleton(right);

    // A single default expression can be matched with a list expression.
    if (left->is<IR::DefaultExpression>() || right->is<IR::DefaultExpression>()) {
        return new IR::BoolLiteral(IR::Type::Boolean::get(), true);
    }

    // If we still have lists after unrolling, compare them.
    if (left->is<IR::BaseListExpression>() || left->is<IR::StructExpression>()) {
        BUG_CHECK(right->is<IR::BaseListExpression>() || right->is<IR::StructExpression>(),
                  "Right expression must be a list expression. Is %1% of type %2%.", right,
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

const IR::Equ *GenEq::mkEq(const IR::Expression *e1, const IR::Expression *e2) {
    return new IR::Equ(IR::Type::Boolean::get(), e1, e2);
}

}  // namespace P4Tools
