#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_STF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_STF_H_

#include <cstddef>
#include <string>
#include <vector>

#include <inja/inja.hpp>

#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/common.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Extracts information from the @testSpec to emit a STF test case.
class STF : public Bmv2TestFramework {
 public:
    explicit STF(const TestBackendConfiguration &testBackendConfiguration);

    /// Produce an STF test.
    void writeTestToFile(const TestSpec *spec, cstring selectedBranches, size_t testId,
                         float currentCoverage) override;

 private:
    /// Emits a test case.
    /// @param testId specifies the test name.
    /// @param selectedBranches enumerates the choices the interpreter made for this path.
    /// @param currentCoverage contains statistics  about the current coverage of this test and its
    /// preceding tests.
    void emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                      const std::string &testCase, float currentCoverage);

    /// @returns the inja test case template as a string.
    static std::string getTestCaseTemplate();

    inja::json getExpectedPacket(const TestSpec *testSpec) const override;

    /// TODO: Fix how BMv2 parses packet strings. We should support hex and octal prefixes.
    inja::json getSend(const TestSpec *testSpec) const override;

    inja::json getControlPlaneForTable(const TableMatchMap &matches,
                                       const std::vector<ActionArg> &args) const override;
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_STF_H_ */
