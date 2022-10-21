#ifndef BACKENDS_P4TOOLS_TESTGEN_OPTIONS_H_
#define BACKENDS_P4TOOLS_TESTGEN_OPTIONS_H_

#include <string>

#include "backends/p4tools/common/options.h"
#include "lib/cstring.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for p4testgen.
class TestgenOptions : public AbstractP4cToolOptions {
 public:
    /// Whether to produce just the input packet for each test.
    bool inputPacketOnly = false;

    /// Maximum number of tests to be generated. Defaults to 1.
    int maxTests = 1;

    /// Fixed packet size in bits. Defaults to 0, which implies no sizing.
    int packetSize = 0;

    /// Selects the exploration strategy for test generation
    std::string explorationStrategy;

    /// Level of multiPop step. A good value is 10, namely, 10 per cent of
    /// the size of the unexploredBranches. The smaller the number,
    /// the bigger the step; e.g. unexploredBranches size == 100
    /// then this variable calculates 100/10 or 100/2 for the pop level.
    /// Defaults to 3, which maximizes exploration of exploration. Minimum
    /// level is 2, for max randomness.
    int popLevel = 3;

    /// Max bound of the buffer vector collecting all terminal branches.
    /// Defaults to 2, which means only two terminal paths are populated
    /// by default.
    int linearEnumeration = 2;

    /// To be used with randomAccessMaxCoverage. It specifies after how many
    /// tests (saddle point) we should randomly explore the program and pick
    /// a random branch ranked by how many unique non-visited statements it
    /// has.
    int saddlePoint = 5;

    /// @returns the singleton instance of this class.
    static TestgenOptions& get();

    /// Directory for generated tests. Defaults to PWD.
    cstring outputDir = nullptr;

    /// Do not fail on unimplemented features. Instead, try the next branch.
    bool permissive = false;

    /// The test back end that P4Testgen will generate test for. Examples, STF, PTF or Protobuf.
    cstring testBackend;

    /// String of selected branches separated by comma.
    std::string selectedBranches;

    /// The count of sorce tests.
    int countOfSourceTests = 0;

    /// Track the branches that are executed in the symbolic executor. This can be used for
    /// deterministic replay of an execution trace.
    bool trackBranches = false;

    /// Build a DCG for input program. This control flow graph directed cyclic graph can be used
    /// for statement reachability analysis.
    bool dcg = false;

    const char* getIncludePath() override;

 private:
    TestgenOptions();
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_OPTIONS_H_ */
