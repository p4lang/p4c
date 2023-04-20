#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BACKEND_PROTOBUF_PROTOBUF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BACKEND_PROTOBUF_PROTOBUF_H_

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <inja/inja.hpp>

#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/lib/tf.h"

namespace P4Tools::P4Testgen::Bmv2 {

using P4::ControlPlaneAPI::p4rt_id_t;
using P4::ControlPlaneAPI::Standard::SymbolType;

/// Extracts information from the @testSpec to emit a Protobuf test case.
class Protobuf : public TF {
 public:
    virtual ~Protobuf() = default;

    Protobuf(const Protobuf &) = delete;

    Protobuf(Protobuf &&) = delete;

    Protobuf &operator=(const Protobuf &) = delete;

    Protobuf &operator=(Protobuf &&) = delete;

    Protobuf(std::filesystem::path basePath, std::optional<unsigned int> seed);

    /// Produce a Protobuf test.
    void outputTest(const TestSpec *spec, cstring selectedBranches, size_t testIdx,
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
    void emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testIdx,
                      const std::string &testCase, float currentCoverage);

    /// @returns the inja test case template as a string.
    static std::string getTestCaseTemplate();

    /// Converts all the control plane objects into Inja format.
    static inja::json getControlPlane(const TestSpec *testSpec);

    /// Converts the input packet and port into Inja format.
    static inja::json getSend(const TestSpec *testSpec);

    /// Converts the output packet, port, and mask into Inja format.
    static inja::json getVerify(const TestSpec *testSpec);

    /// Helper function for @getVerify. Matches the mask value against the input packet value and
    /// generates the appropriate ignore ranges.
    static std::vector<std::pair<size_t, size_t>> getIgnoreMasks(const IR::Constant *mask);

    /// Helper function for the control plane table inja objects.
    static inja::json getControlPlaneForTable(const TableMatchMap &matches,
                                              const std::vector<ActionArg> &args);

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

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BACKEND_PROTOBUF_PROTOBUF_H_ */
