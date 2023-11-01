#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PROTOBUF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PROTOBUF_H_

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <inja/inja.hpp>

#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/common.h"

namespace P4Tools::P4Testgen::Bmv2 {

using P4::ControlPlaneAPI::p4rt_id_t;
using P4::ControlPlaneAPI::Standard::SymbolType;

/// Extracts information from the @testSpec to emit a Protobuf test case.
class Protobuf : public Bmv2TF {
 public:
    explicit Protobuf(std::filesystem::path basePath,
                      std::optional<unsigned int> seed = std::nullopt);

    /// Produce a Protobuf test.
    void outputTest(const TestSpec *spec, cstring selectedBranches, size_t testId,
                    float currentCoverage) override;

 private:
    /// Emits the test preamble. This is only done once for all generated tests.
    /// For the protobuf back end this is the "p4testgen.proto" file.
    void emitPreamble(const std::string &preamble);

    /// Emits a test case.
    /// @param testId specifies the test name.
    /// @param selectedBranches enumerates the choices the interpreter made for this path.
    /// @param currentCoverage contains statistics  about the current coverage of this test and its
    /// preceding tests.
    void emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                      const std::string &testCase, float currentCoverage);

    /// @returns the inja test case template as a string.
    static std::string getTestCaseTemplate();

    inja::json getSend(const TestSpec *testSpec) const override;

    inja::json getControlPlane(const TestSpec *testSpec) const override;

    inja::json getExpectedPacket(const TestSpec *testSpec) const override;

    /// Helper function for the control plane table inja objects.
    inja::json getControlPlaneForTable(const TableMatchMap &matches,
                                       const std::vector<ActionArg> &args) const override;

    /// @return the id allocated to the object through the @id annotation if any, or
    /// std::nullopt.
    static std::optional<p4rt_id_t> getIdAnnotation(const IR::IAnnotated *node);

    /// @return the value of any P4 '@id' annotation @declaration may have, and
    /// ensure that the value is correct with respect to the P4Runtime
    /// specification. The name 'externalId' is in analogy with externalName().
    static std::optional<p4rt_id_t> externalId(const P4::ControlPlaneAPI::P4RuntimeSymbolType &type,
                                               const IR::IDeclaration *declaration);
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_PROTOBUF_H_ */
