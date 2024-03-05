#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_OPTIONS_H_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>

#include "backends/p4tools/common/options.h"
#include "lib/cstring.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"

namespace P4Tools::P4Testgen {

/// Encapsulates and processes command-line options for P4Testgen.
class TestgenOptions : public AbstractP4cToolOptions {
 public:
    virtual ~TestgenOptions() = default;

    /// Maximum number of tests to be generated. Defaults to 1.
    int64_t maxTests = 1;

    /// Selects the path selection policy for test generation
    P4Testgen::PathSelectionPolicy pathSelectionPolicy = P4Testgen::PathSelectionPolicy::DepthFirst;

    /// List of the supported stop metrics.
    static const std::set<cstring> SUPPORTED_STOP_METRICS;

    // Stops generating tests when a particular metric is satisfied. Currently supported options are
    // listed in @var SUPPORTED_STOP_METRICS.
    cstring stopMetric;

    /// @returns the singleton instance of this class.
    static TestgenOptions &get();

    /// Directory for generated tests. Defaults to PWD.
    std::optional<std::filesystem::path> outputDir = std::nullopt;

    /// Fail on unimplemented features instead of trying the next branch
    bool strict = false;

    /// The test back end that P4Testgen will generate test for. Examples are STF, PTF or Protobuf.
    cstring testBackend;

    /// String of selected branches separated by comma.
    std::string selectedBranches;

    /// String of a pattern for resulting tests.
    std::string pattern;

    /// Track the branches that are executed in the symbolic executor. This can be used for
    /// deterministic replay of an execution trace.
    bool trackBranches = false;

    /// Build a DCG for input program. This control flow graph directed cyclic graph can be used
    /// for statement reachability analysis.
    bool dcg = false;

    /// The maximum permitted packet size, in bits.
    // The default is the standard MTU, 1500 bytes.
    int maxPktSize = 12000;

    /// The minimum permitted packet size, in bits.
    int minPktSize = 0;

    /// The list of permitted port ranges.
    /// TestGen will consider these when choosing input and output ports.
    std::vector<std::pair<int, int>> permittedPortRanges;

    /// Skip generating a control plane entry for the entities in this list.
    std::set<cstring> skippedControlPlaneEntities;

    /// Enforces the test generation of tests with mandatory output packet.
    bool outputPacketOnly = false;

    /// Enforces the test generation of tests with mandatory dropped packet.
    bool droppedPacketOnly = false;

    /// Add conditions defined in assert/assume to the path conditions.
    /// Only tests which satisfy these conditions can be generated. This is active by default.
    bool enforceAssumptions = true;

    /// Produce only tests that violate the condition defined in assert calls.
    /// This will either produce no tests or only tests that contain counter examples.
    bool assertionModeEnabled = false;

    /// Specifies general options which IR nodes to track for coverage in the targeted P4 program.
    /// Multiple options are possible. Currently supported: STATEMENTS, TABLE_ENTRIES.
    P4::Coverage::CoverageOptions coverageOptions;

    /// Indicates that coverage tracking is enabled for some coverage criteria. This is used for
    /// sanity checking and it also affects information printed to output.
    bool hasCoverageTracking = false;

    /// Specifies minimum coverage that needs to be achieved for P4Testgen to exit successfully.
    float minCoverage = 0;

    /// The base name of the tests which are generated.
    /// Defaults to the name of the input program, if provided.
    std::optional<cstring> testBaseName;

    const char *getIncludePath() override;

 protected:
    bool validateOptions() const override;

 private:
    TestgenOptions();
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_OPTIONS_H_ */
