#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/targets/bmv2/test/gtest_utils.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"
#include "backends/p4tools/modules/testgen/test/small-step/util.h"

namespace P4::P4Tools::Test {

using SmallStepUtil::extractExpr;
using SmallStepUtil::stepAndExamineOp;

namespace {

/// Test the step function for -(v) unary operation.
TEST_F(Bmv2SmallStepTest, Unary01) {
    const auto test = createSmallStepExprTest("bit<8> f;", "-(hdr.h.f)");
    ASSERT_TRUE(test);

    const auto *opUn = extractExpr<IR::Operation_Unary>(test->getProgram());
    ASSERT_TRUE(opUn);

    // Step on the unary operation and examine the resulting continuation
    // to include the rebuilt IR::Neg node.
    stepAndExamineOp(opUn, opUn->expr, test->getCompilerResult(),
                     [](const IR::PathExpression *expr) { return new IR::Neg(expr); });
}

/// Test the step function for !(v) unary operation.
TEST_F(Bmv2SmallStepTest, Unary02) {
    const auto test = createSmallStepExprTest("bool f;", "!(hdr.h.f)");
    ASSERT_TRUE(test);

    const auto *opUn = extractExpr<IR::Operation_Unary>(test->getProgram());
    ASSERT_TRUE(opUn);

    // Step on the unary operation and examine the resulting continuation
    // to include the rebuilt IR::LNot node.
    stepAndExamineOp(
        opUn, opUn->expr, test->getCompilerResult(),
        [](const IR::PathExpression *expr) { return new IR::LNot(IR::Type_Boolean::get(), expr); });
}

/// Test the step function for ~(v) unary operation.
TEST_F(Bmv2SmallStepTest, Unary03) {
    const auto test = createSmallStepExprTest("bit<8> f;", "~(hdr.h.f)");
    ASSERT_TRUE(test);

    const auto *opUn = extractExpr<IR::Operation_Unary>(test->getProgram());
    ASSERT_TRUE(opUn);

    // Step on the unary operation and examine the resulting continuation
    // to include the rebuilt IR::Cmpl node.
    stepAndExamineOp(opUn, opUn->expr, test->getCompilerResult(),
                     [](const IR::PathExpression *expr) { return new IR::Cmpl(expr); });
}

}  // anonymous namespace

}  // namespace P4::P4Tools::Test
