#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_EBPF_BACKEND_STF_STF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_EBPF_BACKEND_STF_STF_H_

#include <cstddef>
#include <string>
#include <vector>

#include <inja/inja.hpp>

#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::EBPF {

/// Extracts information from the @testSpec to emit a STF test case.
class STF : public TestFramework {
 public:
    ~STF() override = default;
    STF(const STF &) = delete;
    STF(STF &&) = delete;
    STF &operator=(const STF &) = delete;
    STF &operator=(STF &&) = delete;

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

    /// Converts all the control plane objects into Inja format.
    static inja::json getControlPlane(const TestSpec *testSpec);

    /// Converts the input packet and port into Inja format.
    static inja::json getSend(const TestSpec *testSpec);

    /// Converts the output packet, port, and mask into Inja format.
    static inja::json getVerify(const TestSpec *testSpec);

    /// Helper function for the control plane table inja objects.
    static inja::json getControlPlaneForTable(const TableMatchMap &matches,
                                              const std::vector<ActionArg> &args);
};

}  // namespace P4Tools::P4Testgen::EBPF

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_EBPF_BACKEND_STF_STF_H_ */
