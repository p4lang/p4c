#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PTF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PTF_H_

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <inja/inja.hpp>

/// Inja
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/common.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Extracts information from the @testSpec to emit a PTF test case.
class PTF : public Bmv2TestFramework {
 public:
    explicit PTF(const TestBackendConfiguration &testBackendConfiguration);

    /// Produce a PTF test.
    void writeTestToFile(const TestSpec *spec, cstring selectedBranches, size_t testId,
                         float currentCoverage) override;

 private:
    /// Has the preamble been generated already?
    bool preambleEmitted = false;

    /// The output file.
    std::ofstream ptfFileStream;

    /// Emits the test preamble. This is only done once for all generated tests.
    /// For the PTF back end this is the test setup Python script..
    void emitPreamble();

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

    /// Helper function for @getVerify. Matches the mask value against the input packet value and
    /// generates the appropriate ignore ranges.
    static std::vector<std::pair<size_t, size_t>> getIgnoreMasks(const IR::Constant *mask);
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PTF_H_ */
