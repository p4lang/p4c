#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/test/gtest_utils.h"
#include "backends/p4tools/modules/testgen/test/small-step/util.h"

namespace Test {

using SmallStepUtil::createSmallStepExprTest;
using SmallStepUtil::extractExpr;
using SmallStepUtil::stepAndExamineValue;

namespace {

/// Test the step function for a constant.
TEST_F(SmallStepTest, Value01) {
    const auto test = createSmallStepExprTest("bit<8> f;", "8w42");
    ASSERT_TRUE(test);

    const auto *opValue = extractExpr<IR::Constant>(test->getProgram());
    ASSERT_TRUE(opValue);

    // Step on the value and examine the resulting state.
    stepAndExamineValue(opValue, test->getCompilerResult());
}

/// Test the step function for a bool value.
TEST_F(SmallStepTest, Value02) {
    const auto test = createSmallStepExprTest("bit<1> f;", "true");
    ASSERT_TRUE(test);

    const auto *opValue = extractExpr<IR::BoolLiteral>(test->getProgram());
    ASSERT_TRUE(opValue);

    // Step on the value and examine the resulting state.
    stepAndExamineValue(opValue, test->getCompilerResult());
}

}  // anonymous namespace

}  // namespace Test
