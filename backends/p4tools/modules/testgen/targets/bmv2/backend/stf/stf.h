#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BACKEND_STF_STF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BACKEND_STF_STF_H_

#include <cstddef>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <boost/optional/optional.hpp>
#include <inja/inja.hpp>

#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/lib/tf.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/// Extracts information from the @testSpec to emit a STF test case.
class STF : public TF {
    /// The output file.
    std::ofstream stfFile;

 public:
    virtual ~STF() = default;

    STF(const STF&) = delete;

    STF(STF&&) = delete;

    STF& operator=(const STF&) = delete;

    STF& operator=(STF&&) = delete;

    STF(cstring testName, boost::optional<unsigned int> seed);

    /// Produce an STF test.
    void outputTest(const TestSpec* spec, cstring selectedBranches, size_t testIdx,
                    float currentCoverage) override;

 private:
    /// Emits a test case.
    /// @param testId specifies the test name.
    /// @param selectedBranches enumerates the choices the interpreter made for this path.
    /// @param currentCoverage contains statistics  about the current coverage of this test and its
    /// preceding tests.
    void emitTestcase(const TestSpec* testSpec, cstring selectedBranches, size_t testId,
                      const std::string& testCase, float currentCoverage);

    /// Converts all the control plane objects into Inja format.
    static inja::json getControlPlane(const TestSpec* testSpec);

    /// Converts all the counter objects into Inja format.
    static inja::json getCounters(const TestSpec* testSpec);

    /// Converts the input packet and port into Inja format.
    static inja::json getSend(const TestSpec* testSpec);

    /// Converts the output packet, port, and mask into Inja format.
    static inja::json getVerify(const TestSpec* testSpec);

    /// Returns the configuration for a cloned packet configuration.
    static inja::json::array_t getClone(const std::map<cstring, const TestObject*>& cloneInfos);

    /// Helper function for the control plane table inja objects.
    static inja::json getControlPlaneForTable(const std::map<cstring, const FieldMatch>& matches,
                                              const std::vector<ActionArg>& args);
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BACKEND_STF_STF_H_ */
