#include "backends/p4tools/testgen/test/lib/taint.h"

#include <memory>

#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "gtest/gtest-message.h"
#include "gtest/gtest-test-part.h"
#include "gtest/gtest.h"
#include "ir/ir.h"

#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/test/small-step/util.h"

namespace Test {

namespace {

using P4Tools::IRUtils;
using P4Tools::Taint;

/// Test whether taint propagation works at all.
/// Input: 8w2 + taint<8>
/// Expected output: taint<8>
/// Input: 8w2 + 8w2
/// Expected output: 8w2 (since we just collapse untainted binary values with the left side)
/// Input: taint<8> + taint<8>
/// Expected output: taint<8>
TEST_F(TaintTest, Taint01) {
    const auto* typeBits = IRUtils::getBitType(8);
    // Create a base state with a parameter continuation to apply the value on.
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();
    {
        const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
        const auto* constantVar = IRUtils::getConstant(typeBits, 2);
        const auto* expr = new IR::Add(constantVar, taintExpression);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), expr);
        const auto* expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* constantVar1 = IRUtils::getConstant(typeBits, 2);
        const auto* constantVar2 = IRUtils::getConstant(typeBits, 2);
        const auto* expr = new IR::Add(constantVar1, constantVar2);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), expr);
        const auto* expectedExpr = IRUtils::getConstant(typeBits, 2);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
        const auto* expectedExpr = taintExpression;
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
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();

    const auto* typeBits = IRUtils::getBitType(8);
    const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16),
                                          IRUtils::getConstant(typeBits, 2), taintExpression);
        const auto* slicedExpr = new IR::Slice(expr, 15, 8);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = IRUtils::getConstant(typeBits, 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), taintExpression,
                                          IRUtils::getConstant(typeBits, 2));
        const auto* slicedExpr = new IR::Slice(expr, 7, 0);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = IRUtils::getConstant(typeBits, 0);
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
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();

    const auto* typeBits = IRUtils::getBitType(8);
    const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16),
                                          IRUtils::getConstant(typeBits, 2), taintExpression);
        const auto* slicedExpr = new IR::Slice(expr, 7, 0);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), taintExpression,
                                          IRUtils::getConstant(typeBits, 2));
        const auto* slicedExpr = new IR::Slice(expr, 15, 8);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
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
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();

    const auto* typeBits = IRUtils::getBitType(8);
    const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16),
                                          IRUtils::getConstant(typeBits, 2), taintExpression);
        const auto* slicedExpr = new IR::Slice(expr, 11, 4);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), taintExpression,
                                          IRUtils::getConstant(typeBits, 2));
        const auto* slicedExpr = new IR::Slice(expr, 11, 4);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
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
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();

    const auto* typeBits = IRUtils::getBitType(8);
    const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
    const auto* constantVar = IRUtils::getConstant(typeBits, 2);
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IRUtils::getBitType(24), taintExpression, expr);
        const auto* slicedExpr = new IR::Slice(expr, 11, 4);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IRUtils::getBitType(24), taintExpression, expr);
        const auto* slicedExpr = new IR::Slice(expr, 19, 12);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
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
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();

    const auto* typeBits = IRUtils::getBitType(8);
    const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
    const auto* constantVar = IRUtils::getConstant(typeBits, 2);
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IRUtils::getBitType(24), expr, constantVar);
        const auto* slicedExpr = new IR::Slice(expr, 11, 4);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IRUtils::getBitType(24), expr, constantVar);
        const auto* slicedExpr = new IR::Slice(expr, 19, 12);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = taintExpression;
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
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();

    const auto* typeBits = IRUtils::getBitType(8);
    const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
    const auto* constantVar = IRUtils::getConstant(typeBits, 2);

    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IRUtils::getBitType(24), expr, taintExpression);
        const auto* slicedExpr = new IR::Slice(expr, 11, 4);
        slicedExpr = new IR::Slice(slicedExpr, 9, 8);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = IRUtils::getConstant(IRUtils::getBitType(2), 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IRUtils::getBitType(24), expr, taintExpression);
        const auto* slicedExpr = new IR::Slice(expr, 19, 12);
        slicedExpr = new IR::Slice(slicedExpr, 7, 5);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = IRUtils::getConstant(IRUtils::getBitType(3), 0);
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
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();

    const auto* typeBits = IRUtils::getBitType(8);
    const auto* taintExpression = IRUtils::getTaintExpression(typeBits);
    const auto* constantVar = IRUtils::getConstant(typeBits, 2);

    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), taintExpression, constantVar);
        expr = new IR::Concat(IRUtils::getBitType(24), expr, taintExpression);
        const auto* slicedExpr = new IR::Slice(expr, 11, 4);
        slicedExpr = new IR::Slice(slicedExpr, 4, 3);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = IRUtils::getTaintExpression(IRUtils::getBitType(2));
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
    {
        const auto* expr = new IR::Concat(IRUtils::getBitType(16), constantVar, taintExpression);
        expr = new IR::Concat(IRUtils::getBitType(24), expr, taintExpression);
        const auto* slicedExpr = new IR::Slice(expr, 19, 12);
        slicedExpr = new IR::Slice(slicedExpr, 2, 0);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), slicedExpr);
        const auto* expectedExpr = IRUtils::getTaintExpression(IRUtils::getBitType(3));
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
    auto* taint64b = IRUtils::getTaintExpression(IRUtils::getBitType(64));
    ASSERT_TRUE(Taint::hasTaint({}, taint64b));

    // 64w0 ++ taint<64>
    auto* taint128bLowerQ = new IR::Cast(IRUtils::getBitType(128), taint64b);
    ASSERT_TRUE(Taint::hasTaint({}, taint128bLowerQ));
    ASSERT_TRUE(!Taint::hasTaint({}, new IR::Slice(taint128bLowerQ, 71, 64)));

    ASSERT_TRUE(Taint::hasTaint({}, new IR::Slice(taint128bLowerQ, 63, 0)));
    ASSERT_TRUE(!Taint::hasTaint({}, new IR::Slice(taint128bLowerQ, 127, 64)));

    // 32w0 ++ taint<64> ++ 32w0
    auto* taint128bMiddleQ = new IR::Shl(taint128bLowerQ, new IR::Constant(32));
    ASSERT_TRUE(!Taint::hasTaint({}, new IR::Slice(taint128bMiddleQ, 127, 96)));
    ASSERT_TRUE(Taint::hasTaint({}, new IR::Slice(taint128bMiddleQ, 95, 32)));
    ASSERT_TRUE(!Taint::hasTaint({}, new IR::Slice(taint128bMiddleQ, 31, 0)));

    // (32w0 ++ Taint64b ++ 32w0) & 128w0
    // The bitwise and should not have any effect on taint.
    auto* taint128bMiddleQ2 = new IR::BAnd(taint128bMiddleQ, new IR::Constant(128));
    ASSERT_TRUE(!Taint::hasTaint({}, new IR::Slice(taint128bMiddleQ2, 127, 96)));
    ASSERT_TRUE(Taint::hasTaint({}, new IR::Slice(taint128bMiddleQ2, 95, 32)));
    ASSERT_TRUE(!Taint::hasTaint({}, new IR::Slice(taint128bMiddleQ2, 31, 0)));
}

/// Check that taint propagation is not too aggressive.
/// Input: (8w2 + 8w2)[3:0]
/// Expected output: 8w2 (slicing should not cause this expression to be tainted.)
TEST_F(TaintTest, Taint10) {
    const auto* typeBits = IRUtils::getBitType(8);
    // Create a base state with a parameter continuation to apply the value on.
    auto state = ExecutionState(new IR::P4Program());
    const auto& env = state.getSymbolicEnv();
    {
        const auto* constantVar1 = IRUtils::getConstant(typeBits, 2);
        const auto* constantVar2 = IRUtils::getConstant(typeBits, 2);
        const auto* expr = new IR::Slice(new IR::Add(constantVar1, constantVar2), 3, 0);
        const auto* taintedExpr = Taint::propagateTaint(env.getInternalMap(), expr);
        const auto* expectedExpr = IRUtils::getConstant(IRUtils::getBitType(4), 0);
        ASSERT_TRUE(taintedExpr->equiv(*expectedExpr));
    }
}

}  // anonymous namespace

}  // namespace Test
