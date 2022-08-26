#ifndef TESTGEN_TARGETS_BMV2_BACKEND_PROTOBUF_PROTOBUF_H_
#define TESTGEN_TARGETS_BMV2_BACKEND_PROTOBUF_PROTOBUF_H_

#include <cstddef>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/optional/optional.hpp>
#include <inja/inja.hpp>

/// Inja
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/testgen/backend/tf.h"
#include "backends/p4tools/testgen/lib/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/// Extracts information from the @testSpec to emit a Protobuf test case.
class Protobuf : public TF {
    /// Has the preamble been generated already?
    bool preambleEmitted = false;

    /// The output file.
    std::ofstream protobufFile;

 public:
    virtual ~Protobuf() = default;

    Protobuf(const Protobuf&) = delete;

    Protobuf(Protobuf&&) = delete;

    Protobuf& operator=(const Protobuf&) = delete;

    Protobuf& operator=(Protobuf&&) = delete;

    Protobuf(cstring testName, boost::optional<unsigned int> seed);

    void outputTest(const TestSpec* spec, cstring selectedBranches, size_t testIdx,
                    float currentCoverage) override;

 private:
    /// Emits the test preamble. This is only done once for all generated tests.
    /// For the protobuf back end this is the "p4testgen.proto" file.
    void emitPreamble(const std::string& preamble);

    /// Emits a test case.
    /// @param testId specifies the test name.
    /// @param selectedBranches enumerates the choices the interpreter made for this path.
    /// @param currentCoverage contains statistics  about the current coverage of this test and its
    /// preceding tests.
    void emitTestcase(const TestSpec* testSpec, cstring selectedBranches, size_t testId,
                      const std::string& testCase, float currentCoverage);

    /// Converts the traces of this test into a string representation and Inja object.
    static inja::json getTrace(const TestSpec* testSpec);

    /// Converts all the control plane objects into Inja format.
    static inja::json getControlPlane(const TestSpec* testSpec);

    /// Converts the input packet and port into Inja format.
    static inja::json getSend(const TestSpec* testSpec);

    /// Converts the output packet, port, and mask into Inja format.
    static inja::json getVerify(const TestSpec* testSpec);

    /// Helper function for @getVerify. Matches the mask value against the input packet value and
    /// generates the appropriate ignore ranges.
    static std::vector<std::pair<size_t, size_t>> getIgnoreMasks(const IR::Constant* mask);

    /// Helper function for the control plane table inja objects.
    static inja::json getControlPlaneForTable(const std::map<cstring, const FieldMatch>& matches,
                                              const std::vector<ActionArg>& args);
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_BACKEND_PROTOBUF_PROTOBUF_H_ */
