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
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/lib/tf.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

class TestBackEnd {
 private:
    /// The current test count. If it exceeds @var maxTests, the symbolic executor will stop.
    int64_t testCount = 0;

    /// Indicates the number of generated tests after which we reset memory.
    static const int64_t RESET_THRESHOLD = 10000;

 protected:
    /// ProgramInfo is used to access some target specific information for test generation.
    const ProgramInfo &programInfo;

    /// Writes the tests out to a file.
    TF *testWriter = nullptr;

    /// Pointer to the symbolic executor.
    /// TODO: Remove this.
    SymbolicExecutor &symbex;

    /// Test maximum number of tests that are to be produced.
    int64_t maxTests;

    explicit TestBackEnd(const ProgramInfo &programInfo, SymbolicExecutor &symbex)
        : programInfo(programInfo), symbex(symbex), maxTests(TestgenOptions::get().maxTests) {
        // If we select a specific branch, the number of tests should be 1.
        if (!TestgenOptions::get().selectedBranches.empty()) {
            maxTests = 1;
        }
    }

    [[nodiscard]] bool needsToTerminate(int64_t testCount) const { return testCount == maxTests; }

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

    /// Print out some performance numbers if logging feature "performance" is enabled.
    /// Also log performance numbers to a separate file in the test folder if @param write is
    /// enabled.
    void printPerformanceReport(bool write) const;

    /// Accessors.
    [[nodiscard]] int64_t getTestCount() const;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_BACKEND_H_ */
