#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/targets/bmv2/test/gtest_utils.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"
#include "backends/p4tools/modules/testgen/test/small-step/util.h"

namespace P4::P4Tools::Test {

using SmallStepUtil::extractExpr;
using SmallStepUtil::stepAndExamineValue;

namespace {

/// Test the step function for a constant.
TEST_F(Bmv2SmallStepTest, Value01) {
    const auto test = createBmv2V1modelSmallStepExprTest("bit<8> f;", "8w42");
    ASSERT_TRUE(test);

    const auto *opValue = extractExpr<IR::Constant>(test->getProgram());
    ASSERT_TRUE(opValue);

    // Step on the value and examine the resulting state.
    stepAndExamineValue(opValue, test->getCompilerResult());
}

/// Test the step function for a bool value.
TEST_F(Bmv2SmallStepTest, Value02) {
    const auto test = createBmv2V1modelSmallStepExprTest("bit<1> f;", "true");
    ASSERT_TRUE(test);

    const auto *opValue = extractExpr<IR::BoolLiteral>(test->getProgram());
    ASSERT_TRUE(opValue);

    // Step on the value and examine the resulting state.
    stepAndExamineValue(opValue, test->getCompilerResult());
}

}  // anonymous namespace

}  // namespace P4::P4Tools::Test
