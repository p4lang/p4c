#include <gtest/gtest.h>

#include <optional>

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/test/gtest_utils.h"
#include "backends/p4tools/modules/testgen/test/small-step/util.h"

namespace Test {

using SmallStepUtil::createSmallStepExprTest;
using SmallStepUtil::extractExpr;
using SmallStepUtil::stepAndExamineOp;

namespace {

/// Test the step function for v + e binary operation.
TEST_F(SmallStepTest, Binary01) {
    const auto test = createSmallStepExprTest("bit<8> f;", "8w42 + hdr.h.f");
    ASSERT_TRUE(test);

    const auto *opBin = extractExpr<IR::Operation_Binary>(test->getProgram());
    ASSERT_TRUE(opBin);

    // Step on the binary operation and examine the resulting continuation
    // to include the rebuilt IR::Add node.
    stepAndExamineOp(
        opBin, opBin->right, test->getCompilerResult(),
        [opBin](const IR::PathExpression *expr) { return new IR::Add(opBin->left, expr); });
}

/// Test the step function for e + e binary operation.
TEST_F(SmallStepTest, Binary02) {
    const auto test = createSmallStepExprTest(R"(
                                               bit<8> f1;
                                               bit<8> f2;)",
                                              "hdr.h.f1 + hdr.h.f2");
    ASSERT_TRUE(test);

    const auto *opBin = extractExpr<IR::Operation_Binary>(test->getProgram());
    ASSERT_TRUE(opBin);

    // Step on the binary operation and examine the resulting continuation
    // to include the rebuilt IR::Add node.
    stepAndExamineOp(
        opBin, opBin->left, test->getCompilerResult(),
        [opBin](const IR::PathExpression *expr) { return new IR::Add(expr, opBin->right); });
}

/// Test the step function for e == v binary operation.
TEST_F(SmallStepTest, Binary03) {
    const auto test = createSmallStepExprTest("bit<8> f;", "hdr.h.f == 8w42");
    ASSERT_TRUE(test);

    const auto *opBin = extractExpr<IR::Operation_Binary>(test->getProgram());
    ASSERT_TRUE(opBin);

    // Step on the binary operation and examine the resulting continuation
    // to include the rebuilt IR::Equ node.
    stepAndExamineOp(
        opBin, opBin->left, test->getCompilerResult(),
        [opBin](const IR::PathExpression *expr) { return new IR::Equ(expr, opBin->right); });
}

/// Test the step function for v ++ e binary operation.
TEST_F(SmallStepTest, Binary04) {
    const auto test = createSmallStepExprTest("bit<8> f;", "8w42 ++ hdr.h.f");
    ASSERT_TRUE(test);

    const auto *opBin = extractExpr<IR::Operation_Binary>(test->getProgram());
    ASSERT_TRUE(opBin);

    // Step on the binary operation and examine the resulting continuation
    // to include the rebuilt IR::Concat node.
    stepAndExamineOp(
        opBin, opBin->right, test->getCompilerResult(),
        [opBin](const IR::PathExpression *expr) { return new IR::Concat(opBin->left, expr); });
}

}  // anonymous namespace

}  // namespace Test
