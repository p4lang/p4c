#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_BACKEND_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_BACKEND_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/final_state.h"
#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

class TestBackEnd {
 private:
    /// The current test count. If it exceeds @var maxTests, the symbolic executor will stop.
    int64_t testCount = 0;

    /// Indicates the number of generated tests after which we reset memory.
    static const int64_t RESET_THRESHOLD = 10000;

    /// ProgramInfo is used to access some target specific information for test generation.
    std::reference_wrapper<const ProgramInfo> programInfo;

    /// Configuration options for the test back end.
    std::reference_wrapper<const TestBackendConfiguration> testBackendConfiguration;

 protected:
    /// Writes the tests out to a file.
    TestFramework *testWriter = nullptr;

    /// Pointer to the symbolic executor.
    /// TODO: Remove this. We only need to update coverage tracking.
    SymbolicExecutor &symbex;

    /// Test maximum number of tests that are to be produced.
    int64_t maxTests;

    /// The accumulated coverage of all finished test cases. Number in range [0, 1].
    float coverage = 0;

    /// The list of tests accumulated in the test back end.
    AbstractTestList tests;

    explicit TestBackEnd(const ProgramInfo &programInfo,
                         const TestBackendConfiguration &testBackendConfiguration,
                         SymbolicExecutor &symbex);

    [[nodiscard]] bool needsToTerminate(int64_t testCount) const;

 public:
    TestBackEnd(const TestBackEnd &) = default;

    TestBackEnd(TestBackEnd &&) = default;

    TestBackEnd &operator=(const TestBackEnd &) = delete;

    TestBackEnd &operator=(TestBackEnd &&) = delete;

    virtual ~TestBackEnd() = default;

    struct TestInfo {
        /// The concrete value of the input packet.
        /// This is a slice of the program packet according to packetSizeInInt.
        const IR::Constant *inputPacket;

        /// The input port of the packet.
        int inputPort;

        /// The concrete value of the output packet as modified by the packet.
        const IR::Constant *outputPacket;

        /// The output port of the packet.
        int outputPort;

        /// The taint mask.
        const IR::Constant *packetTaintMask;

        /// The traces that have been collected during execution of this particular test path.
        const std::vector<std::reference_wrapper<const TraceEvent>> programTraces;

        /// Indicates whether the packet is dropped.
        bool packetIsDropped = false;
    };

    /// @returns the test specification which is consumed by the test back ends.
    virtual const TestSpec *createTestSpec(const ExecutionState *executionState,
                                           const Model *finalModel, const TestInfo &testInfo) = 0;

    /// Prints information about this particular test path.
    /// @returns false if the test generation is to be aborted (for example when the port is
    /// tainted.)
    virtual bool printTestInfo(const ExecutionState *executionState, const TestInfo &testInfo,
                               const IR::Expression *outputPortExpr);

    /// @returns a new modules with all concolic variables in the program resolved.
    [[nodiscard]] std::optional<std::reference_wrapper<const FinalState>> computeConcolicVariables(
        const FinalState &state) const;

    /// @returns a TestInfo objects, which contains information about the input/output ports, the
    /// taint mask, the packet sizes, etc...
    virtual TestInfo produceTestInfo(
        const ExecutionState *executionState, const Model *finalModel,
        const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
        const std::vector<std::reference_wrapper<const TraceEvent>> *programTraces);

    /// The callback that is executed by the symbolic executor.
    virtual bool run(const FinalState &state);

    /// Returns test count.
    [[nodiscard]] int64_t getTestCount() const;

    /// Returns coverage achieved by all the processed tests.
    [[nodiscard]] float getCoverage() const;

    /// Returns the program info.
    [[nodiscard]] const ProgramInfo &getProgramInfo() const;

    /// Returns the configuration options for the test back end.
    [[nodiscard]] const TestBackendConfiguration &getTestBackendConfiguration() const;

    /// Returns the list of tests accumulated in the test back end.
    /// If the test write is in file mode this list will be empty.
    [[nodiscard]] const AbstractTestList &getTests() const { return tests; }
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_BACKEND_H_ */
