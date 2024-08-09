#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_GTEST_UTILS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_GTEST_UTILS_H_

#include "backends/p4tools/modules/testgen/test/gtest_utils.h"
#include "backends/p4tools/modules/testgen/test/small-step/util.h"

namespace P4Tools::Test {

/// Sets up the correct context for a P4Testgen BMv2 test.
class P4TestgenBmv2Test : public P4TestgenTest {};

/// Creates a test case with the @hdrFields for stepping on an @expr.
std::optional<const P4ToolsTestCase> createSmallStepExprTest(const std::string &hdrFields,
                                                             const std::string &expr);

/// BMv2-specific version of a small step test.
class Bmv2SmallStepTest : public SmallStepTest {};

}  // namespace P4Tools::Test

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_GTEST_UTILS_H_ */
