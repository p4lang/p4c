#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PROTOBUF_IR_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PROTOBUF_IR_H_

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include <inja/inja.hpp>

#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/common.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Extracts information from the @testSpec to emit a Protobuf IR test case.
class ProtobufIr : public Bmv2TestFramework {
 public:
    explicit ProtobufIr(std::filesystem::path basePath,
                        std::optional<unsigned int> seed = std::nullopt);

    /// Produce a ProtobufIr test.
    void outputTest(const TestSpec *spec, cstring selectedBranches, size_t testId,
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
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PROTOBUF_IR_H_ */
