#include "backends/p4tools/modules/testgen/test/lib/taint.h"

#include <gtest/gtest.h>

#include <memory>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir.h"
#include "ir/irutils.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/test/small-step/util.h"

namespace Test {

namespace {

using P4Tools::Taint;

/// Test whether taint propagation works at all.
/// Input: 8w2 + taint<8>
/// Expected output: taint<8>
/// Input: 8w2 + 8w2
/// Expected output: 8w2 (since we just collapse untainted binary values with the left side)
/// Input: taint<8> + taint<8>
/// Expected output: taint<8>
TEST_F(TaintTest, Taint01) {
    const auto *typeBits = IR::getBitType(8);
    // Create a base state with a parameter continuation to apply the value on.
    {
        const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
        const auto *constantVar = IR::getConstant(typeBits, 2);
        const auto *expr = new IR::Add(constantVar, taintExpression);
        const auto *taintedExpr = Taint::propagateTaint(expr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *constantVar1 = IR::getConstant(typeBits, 2);
        const auto *constantVar2 = IR::getConstant(typeBits, 2);
        const auto *expr = new IR::Add(constantVar1, constantVar2);
        const auto *taintedExpr = Taint::propagateTaint(expr);
        const auto *expectedExpr = IR::getConstant(typeBits, 2);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintExpression->equiv(*expectedExpr));
    }
}

/// Test whether taint propagation respects slices and concatenation.
/// Input: (8w2 ++ taint<8>)[15:8]
/// Expected output: 8w0 (since we replace values with zeroes)
/// Input: (taint<8> ++ 8w2 )[7:0]
/// Expected output: 8w0 (since we replace values with zeroes)
TEST_F(TaintTest, Taint02) {
    // Create a base state with a parameter continuation to apply the value on.
    const auto *typeBits = IR::getBitType(8);
    const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
    {
        const auto *expr =
            new IR::Concat(IR::getBitType(16), IR::getConstant(typeBits, 2), taintExpression);
        const auto *slicedExpr = new IR::Slice(expr, 15, 8);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = IR::getConstant(typeBits, 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *expr =
            new IR::Concat(IR::getBitType(16), taintExpression, IR::getConstant(typeBits, 2));
        const auto *slicedExpr = new IR::Slice(expr, 7, 0);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = IR::getConstant(typeBits, 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

/// Test whether taint propagation respects slices and concatenation.
/// Input: (8w2 ++ taint<8>)[7:0]
/// Expected output: taint<8>
/// Input: (taint<8> ++ 8w2 )[15:8]
/// Expected output: taint<8>
TEST_F(TaintTest, Taint03) {
    // Create a base state with a parameter continuation to apply the value on.
    const auto *typeBits = IR::getBitType(8);
    const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
    {
        const auto *expr =
            new IR::Concat(IR::getBitType(16), IR::getConstant(typeBits, 2), taintExpression);
        const auto *slicedExpr = new IR::Slice(expr, 7, 0);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *expr =
            new IR::Concat(IR::getBitType(16), taintExpression, IR::getConstant(typeBits, 2));
        const auto *slicedExpr = new IR::Slice(expr, 15, 8);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

/// Test whether taint propagation respects slices and concatenation.
/// Input: (8w2 ++ taint<8>)[11:4]
/// Expected output: taint<8>
/// Input: (taint<8> ++ 8w2 )[11:4]
/// Expected output: taint<8>
TEST_F(TaintTest, Taint04) {
    // Create a base state with a parameter continuation to apply the value on.
    const auto *typeBits = IR::getBitType(8);
    const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
    {
        const auto *expr =
            new IR::Concat(IR::getBitType(16), IR::getConstant(typeBits, 2), taintExpression);
        const auto *slicedExpr = new IR::Slice(expr, 11, 4);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *expr =
            new IR::Concat(IR::getBitType(16), taintExpression, IR::getConstant(typeBits, 2));
        const auto *slicedExpr = new IR::Slice(expr, 11, 4);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

/// Test whether taint propagation respects slices and concatenation.
/// Input: (taint<8> ++ 8w2 ++ taint<8>) [11:4]
/// Expected output: taint<8>
/// Input: (taint<8> ++ 8w2 ++ taint<8>)[19:12]
/// Expected output: taint<8>
TEST_F(TaintTest, Taint05) {
    // Create a base state with a parameter continuation to apply the value on.
    const auto *typeBits = IR::getBitType(8);
    const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
    const auto *constantVar = IR::getConstant(typeBits, 2);
    {
        const auto *expr = new IR::Concat(IR::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IR::getBitType(24), taintExpression, expr);
        const auto *slicedExpr = new IR::Slice(expr, 11, 4);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *expr = new IR::Concat(IR::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IR::getBitType(24), taintExpression, expr);
        const auto *slicedExpr = new IR::Slice(expr, 19, 12);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

/// Test whether taint propagation respects slices and concatenation.
/// Input: (8w2 ++ taint<8> ++ 8w2) [11:4]
/// Expected output: taint<8>
/// Input: (8w2 ++ taint<8> ++ 8w2)[19:12]
/// Expected output: taint<8>
TEST_F(TaintTest, Taint06) {
    // Create a base state with a parameter continuation to apply the value on.
    const auto *typeBits = IR::getBitType(8);
    const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
    const auto *constantVar = IR::getConstant(typeBits, 2);
    {
        const auto *expr = new IR::Concat(IR::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IR::getBitType(24), expr, constantVar);
        const auto *slicedExpr = new IR::Slice(expr, 11, 4);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *expr = new IR::Concat(IR::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IR::getBitType(24), expr, constantVar);
        const auto *slicedExpr = new IR::Slice(expr, 19, 12);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

/// Test whether taint propagation respects slices and concatenation.
/// Input: (taint<8> ++ 8w2 ++ taint<8>) [11:4][9:8]
/// Expected output: 2w0
/// Input: (8w2 ++ taint<8> ++ taint<8>)[19:12][7:5]
/// Expected output: 2w0
TEST_F(TaintTest, Taint07) {
    // Create a base state with a parameter continuation to apply the value on.
    const auto *typeBits = IR::getBitType(8);
    const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
    const auto *constantVar = IR::getConstant(typeBits, 2);

    {
        const auto *expr = new IR::Concat(IR::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IR::getBitType(24), expr, taintExpression);
        const auto *slicedExpr = new IR::Slice(expr, 11, 4);
        slicedExpr = new IR::Slice(slicedExpr, 9, 8);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = IR::getConstant(IR::getBitType(2), 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *expr = new IR::Concat(IR::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IR::getBitType(24), expr, taintExpression);
        const auto *slicedExpr = new IR::Slice(expr, 19, 12);
        slicedExpr = new IR::Slice(slicedExpr, 7, 5);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = IR::getConstant(IR::getBitType(3), 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}
/// Test whether taint propagation respects slices and concatenation.
/// Input: (taint<8> ++ 8w2 ++ taint<8>) [11:4][4:3]
/// Expected output: taint<2>
/// Input: (8w2 ++ taint<8> ++ taint<8>)[19:12][2:0]
/// Expected output: taint<3>
TEST_F(TaintTest, Taint08) {
    // Create a base state with a parameter continuation to apply the value on.
    const auto *typeBits = IR::getBitType(8);
    const auto *taintExpression = P4Tools::ToolsVariables::getTaintExpression(typeBits);
    const auto *constantVar = IR::getConstant(typeBits, 2);

    {
        const auto *expr = new IR::Concat(IR::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IR::getBitType(24), expr, taintExpression);
        const auto *slicedExpr = new IR::Slice(expr, 11, 4);
        slicedExpr = new IR::Slice(slicedExpr, 4, 3);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = P4Tools::ToolsVariables::getTaintExpression(IR::getBitType(2));
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto *expr = new IR::Concat(IR::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IR::getBitType(24), expr, taintExpression);
        const auto *slicedExpr = new IR::Slice(expr, 19, 12);
        slicedExpr = new IR::Slice(slicedExpr, 2, 0);
        const auto *taintedExpr = Taint::propagateTaint(slicedExpr);
        const auto *expectedExpr = P4Tools::ToolsVariables::getTaintExpression(IR::getBitType(3));
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

/// Test whether taint detections respects slices, casts and shifts.
/// Input: (bit<128>)(taint<64>)
/// Expected output: taint in lower 64 bits, no taint in upper 64 bits
/// Input: 32w0 ++ (taint<64>) ++ 32w0
/// Expected output: taint in most upper and lower 32 bits, taint in middle 64 bits
/// Input: (32w0 ++ (taint<64>) ++ 32w0) & 128w0
/// Expected output: taint in most upper and lower 32 bits, taint in middle 64 bits
TEST_F(TaintTest, Taint09) {
    // Taint64b
    const auto *taint64b = P4Tools::ToolsVariables::getTaintExpression(IR::getBitType(64));
    ASSERT_TRUE(Taint::hasTaint(taint64b));

    ASSERT_TRUE(Taint::hasTaint(new IR::Slice(taint64b, 0, 0)));
    ASSERT_TRUE(Taint::hasTaint(new IR::Slice(new IR::Slice(taint64b, 0, 0), 0, 0)));

    // 64w0 ++ taint<64>
    const auto *taint128bLowerQ = new IR::Cast(IR::getBitType(128), taint64b);
    ASSERT_TRUE(Taint::hasTaint(taint128bLowerQ));
    ASSERT_TRUE(!Taint::hasTaint(new IR::Slice(taint128bLowerQ, 71, 64)));

    ASSERT_TRUE(Taint::hasTaint(new IR::Slice(taint128bLowerQ, 63, 0)));
    ASSERT_TRUE(!Taint::hasTaint(new IR::Slice(taint128bLowerQ, 127, 64)));

    // 32w0 ++ taint<64> ++ 32w0
    const auto *taint128bMiddleQ = new IR::Shl(taint128bLowerQ, new IR::Constant(32));
    ASSERT_TRUE(!Taint::hasTaint(new IR::Slice(taint128bMiddleQ, 127, 96)));
    ASSERT_TRUE(Taint::hasTaint(new IR::Slice(taint128bMiddleQ, 95, 32)));
    ASSERT_TRUE(!Taint::hasTaint(new IR::Slice(taint128bMiddleQ, 31, 0)));

    // (32w0 ++ Taint64b ++ 32w0) & 128w0
    // The bitwise and should not have any effect on taint.
    const auto *taint128bMiddleQ2 = new IR::BAnd(taint128bMiddleQ, new IR::Constant(128));
    ASSERT_TRUE(!Taint::hasTaint(new IR::Slice(taint128bMiddleQ2, 127, 96)));
    ASSERT_TRUE(Taint::hasTaint(new IR::Slice(taint128bMiddleQ2, 95, 32)));
    ASSERT_TRUE(!Taint::hasTaint(new IR::Slice(taint128bMiddleQ2, 31, 0)));
}

/// Check that taint propagation is not too aggressive.
/// Input: (8w2 + 8w2)[3:0]
/// Expected output: 8w2 (slicing should not cause this expression to be tainted.)
TEST_F(TaintTest, Taint10) {
    const auto *typeBits = IR::getBitType(8);
    // Create a base state with a parameter continuation to apply the value on.
    {
        const auto *constantVar1 = IR::getConstant(typeBits, 2);
        const auto *constantVar2 = IR::getConstant(typeBits, 2);
        const auto *expr = new IR::Slice(new IR::Add(constantVar1, constantVar2), 3, 0);
        const auto *taintedExpr = Taint::propagateTaint(expr);
        const auto *expectedExpr = IR::getConstant(IR::getBitType(4), 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

}  // anonymous namespace

}  // namespace Test
