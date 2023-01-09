#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_OPTIONS_H_

#include <string>

#include "backends/p4tools/common/options.h"
#include "lib/cstring.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for p4testgen.
class TestgenOptions : public AbstractP4cToolOptions {
 public:
    /// Maximum number of tests to be generated. Defaults to 1.
    int64_t maxTests = 1;

    /// List of the supported exploration strategies.
    static const std::set<cstring> SUPPORTED_EXPLORATION_STRATEGIES;

    /// Selects the exploration strategy for test generation
    cstring explorationStrategy;

    /// List of the supported stop metrics.
    static const std::set<cstring> SUPPORTED_STOP_METRICS;

    // Stops generating tests when a particular metric is satisifed. Currently supported options are
    // listed in @var SUPPORTED_STOP_METRICS.
    cstring stopMetric;

    /// Level of multiPop step. A good value is 10, namely, 10 per cent of
    /// the size of the unexploredBranches. The smaller the number,
    /// the bigger the step; e.g. unexploredBranches size == 100
    /// then this variable calculates 100/10 or 100/2 for the pop level.
    /// Defaults to 3, which maximizes exploration of exploration. Minimum
    /// level is 2, for max randomness.
    uint64_t popLevel = 3;

    /// Max bound of the buffer vector collecting all terminal branches.
    /// Defaults to 2, which means only two terminal paths are populated
    /// by default.
    uint64_t linearEnumeration = 2;

    /// To be used with randomAccessMaxCoverage. It specifies after how many
    /// tests (saddle point) we should randomly explore the program and pick
    /// a random branch ranked by how many unique non-visited statements it
    /// has.
    uint64_t saddlePoint = 5;

    /// @returns the singleton instance of this class.
    static TestgenOptions& get();

    /// Directory for generated tests. Defaults to PWD.
    cstring outputDir = nullptr;

    /// Fail on unimplemented features instead of trying the next branch
    bool strict = false;

    /// The test back end that P4Testgen will generate test for. Examples, STF, PTF or Protobuf.
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

    /// Enforces the test generation of tests with mandatory output packet.
    bool withOutputPacket = false;

    const char* getIncludePath() override;

 private:
    TestgenOptions();
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_OPTIONS_H_ */
