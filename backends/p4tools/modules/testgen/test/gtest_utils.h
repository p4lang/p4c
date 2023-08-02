#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_GTEST_UTILS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_GTEST_UTILS_H_

#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "backends/p4tools/common/lib/variables.h"
#include "frontends/common/options.h"
#include "ir/ir.h"

namespace Test {

/// Encapsulates functionality for running the front- and mid-ends of the compiler in a test.
class P4ToolsTestCase {
 public:
    /// Factory method for producing a test case from a P4 program source.
    static std::optional<const P4ToolsTestCase> create(std::string deviceName, std::string archName,
                                                       CompilerOptions::FrontendVersion langVersion,
                                                       const std::string &source);

    /// Factory method for producing a test case from a P4_14 program source.
    static std::optional<const P4ToolsTestCase> create_14(std::string deviceName,
                                                          std::string archName,
                                                          const std::string &source);

    /// Factory method for producing a test case from a P4_16 program source.
    static std::optional<const P4ToolsTestCase> create_16(std::string deviceName,
                                                          std::string archName,
                                                          const std::string &source);

    /// The output of the compiler's mid end.
    const IR::P4Program *program;

 private:
    /// Ensures target plug-ins are initialized.
    static void ensureInit();
};

/// GTest for P4 Tools tests.
class P4ToolsTest : public ::testing::Test {};

/// Converts IR::Member into symbolic variables.
class SymbolicConverter : public Transform {
 public:
    const IR::SymbolicVariable *preorder(IR::Member *member) override;
};

}  // namespace Test

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_GTEST_UTILS_H_ */
