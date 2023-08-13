#include "midend/saturationElim.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/null.h"

namespace P4 {

bool SaturationElim::isSaturationOperation(const IR::Expression *expr) {
    CHECK_NULL(expr);
    return expr->is<IR::AddSat>() || expr->is<IR::SubSat>();
}

const IR::Mux *SaturationElim::eliminate(const IR::Operation_Binary *binary) {
    BUG_CHECK(isSaturationOperation(binary),
              "Can't do saturation elimination on a non-saturating operation: %1%", binary);

    const auto *bitType = binary->type->to<IR::Type_Bits>();
    BUG_CHECK(bitType, "Unsupported type for conversion %1%", binary->type);

    // overflowNumber is the largest value that can be represented before overflowing.
    // This is 2^(bits->size) - 1 for unsigned and 2^(bits->size - 1) - 1 for signed.
    const auto *overflowNumber = IR::getConstant(bitType, IR::getMaxBvVal(bitType));

    const IR::Constant *zero = IR::getConstant(bitType, 0);

    // underflowNumber is the smallest value that can be represented before underflowing.
    // This is 0 for unsigned and -(2^(bits->size - 1)) for signed.
    const auto *underflowNumber =
        (bitType->isSigned) ? IR::getConstant(bitType, IR::getMinBvVal(bitType)) : zero;

    const auto *boolType = IR::Type::Boolean::get();
    const IR::Expression *expr = nullptr;
    const IR::Expression *overflowCondition = nullptr;
    const IR::Expression *underflowCondition = nullptr;

    if (binary->is<IR::AddSat>()) {
        expr = new IR::Add(bitType, binary->left, binary->right);

        if (bitType->isSigned) {
            // We have a non-overflowing addition, x + y, of two signed numbers exactly when
            // (x > 0 && y > 0) => (x + y > 0).
            //
            // Therefore, the addition overflows exactly when
            // !((x > 0 && y > 0) => (x + y > 0))
            //   <=> !(!(x > 0 && y > 0) || (x + y > 0))
            //   <=> x > 0 && y > 0 && x + y <= 0
            const auto *leftPositive = new IR::Grt(boolType, binary->left, zero);
            const auto *rightPositive = new IR::Grt(boolType, binary->right, zero);
            const auto *exprNonPositive = new IR::Leq(boolType, expr, zero);
            overflowCondition = new IR::LAnd(
                boolType, leftPositive, new IR::LAnd(boolType, rightPositive, exprNonPositive));

            // We have a non-underflowing addition, x + y, of two signed numbers exactly when
            // (x < 0 && y < 0) => (x + y < 0).
            //
            // Therefore, the addition underflows exactly when
            // !((x < 0 && y < 0) => (x + y < 0))
            //   <=> !(!(x < 0 && y < 0) || (x + y < 0))
            //   <=> x < 0 && y < 0 && x + y >= 0
            const auto *leftNegative = new IR::Lss(boolType, binary->left, zero);
            const auto *rightNegative = new IR::Lss(boolType, binary->right, zero);
            const auto *exprNonNegative = new IR::Geq(boolType, expr, zero);
            underflowCondition = new IR::LAnd(boolType, exprNonNegative,
                                              new IR::LAnd(boolType, leftNegative, rightNegative));
        } else {
            // For an unsigned addition, x + y, overflow occurs exactly when
            // x > overflowNumber - y.
            overflowCondition = new IR::Grt(boolType, binary->left,
                                            new IR::Sub(bitType, overflowNumber, binary->right));

            // Unsigned addition never underflows.
            underflowCondition = IR::getBoolLiteral(false);
        }
    } else if (binary->is<IR::SubSat>()) {
        expr = new IR::Sub(bitType, binary->left, binary->right);

        if (bitType->isSigned) {
            // Detection of overflowing subtraction, x - y, of two signed numbers works similarly
            // to detecting overflow in the addition of x + (-y). Based on the above reasoning for
            // addition, we have overflow when
            //
            //   x > 0 && -y > 0 && x - y <= 0
            //     <=> x > 0 && y <= 0 && x - y <= 0
            const auto *leftPositive = new IR::Grt(boolType, binary->left, zero);
            const auto *rightNonPositive = new IR::Leq(boolType, binary->right, zero);
            const auto *exprNonPositive = new IR::Leq(boolType, expr, zero);
            overflowCondition = new IR::LAnd(
                boolType, leftPositive, new IR::LAnd(boolType, rightNonPositive, exprNonPositive));

            // Detection of underflowing subtraction, x - y, of two signed numbers works similarly
            // to detecting underflow in the addition of x + (-y). Based on the above reasoning for
            // addition, we have underflow when
            //
            //   x < 0 && -y < 0 && x - y >= 0
            //     <=> x < 0 && y >= 0 && x - y >= 0
            const auto *leftNegative = new IR::Lss(boolType, binary->left, zero);
            const auto *rightNonNegative = new IR::Geq(boolType, binary->right, zero);
            const auto *exprNonNegative = new IR::Geq(boolType, expr, zero);
            underflowCondition = new IR::LAnd(
                boolType, leftNegative, new IR::LAnd(boolType, rightNonNegative, exprNonNegative));
        } else {
            // Unsigned subtraction never overflows.
            overflowCondition = IR::getBoolLiteral(false);

            // An unsigned subtraction, x - y, underflows exactly when x < y.
            underflowCondition = new IR::Lss(boolType, binary->left, binary->right);
        }
    } else {
        BUG("EliminateSaturation : unsuported operation %1%", binary);
    }

    BUG_CHECK(expr != nullptr, "Invalid pointer of addition/subtraction calculation result");

    return new IR::Mux(bitType, overflowCondition, overflowNumber,
                       new IR::Mux(binary->type, underflowCondition, underflowNumber, expr));
}

}  // namespace P4
