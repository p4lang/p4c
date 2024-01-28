#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_BACKEND_METADATA_METADATA_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_BACKEND_METADATA_METADATA_H_

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <inja/inja.hpp>

#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

/// Extracts information from the @testSpec to emit a Metadata test case.
class Metadata : public TestFramework {
    /// The output file.
    std::ofstream metadataFile;

 public:
    ~Metadata() override = default;
    Metadata(const Metadata &) = delete;
    Metadata(Metadata &&) = delete;
    Metadata &operator=(const Metadata &) = delete;
    Metadata &operator=(Metadata &&) = delete;

    explicit Metadata(const TestBackendConfiguration &testBackendConfiguration);

    /// Produce a Metadata test.
    void writeTestToFile(const TestSpec *spec, cstring selectedBranches, size_t testId,
                         float currentCoverage) override;

 private:
    /// Emits the test preamble. This is only done once for all generated tests.
    /// For the Metadata back end this is the "p4testgen.proto" file.
    void emitPreamble(const std::string &preamble);

    /// Emits a test case.
    /// @param testId specifies the test name.
    /// @param selectedBranches enumerates the choices the interpreter made for this path.
    /// @param currentCoverage contains statistics  about the current coverage of this test and its
    /// preceding tests.
    void emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                      const std::string &testCase, float currentCoverage);

    /// Gets the traces from @param testSpec and populates @param dataJson.
    /// Also retrieves the label and offset for each successful extract call and stores them in a
    /// "offsets" key.
    static void computeTraceData(const TestSpec *testSpec, inja::json &dataJson);

    /// @returns the inja test case template as a string.
    static std::string getTestCaseTemplate();

    /// Converts the input packet and port into Inja format.
    static inja::json getSend(const TestSpec *testSpec);

    /// Converts the output packet, port, and mask into Inja format.
    static inja::json getVerify(const TestSpec *testSpec);

    /// Helper function for @getVerify. Matches the mask value against the input packet value and
    /// generates the appropriate ignore ranges.
    static std::vector<std::pair<size_t, size_t>> getIgnoreMasks(const IR::Constant *mask);
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_BACKEND_METADATA_METADATA_H_ */
