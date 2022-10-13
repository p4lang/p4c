#ifndef BACKENDS_P4TOOLS_TESTGEN_LIB_TEST_BACKEND_H_
#define BACKENDS_P4TOOLS_TESTGEN_LIB_TEST_BACKEND_H_

#include <string>
#include <vector>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/coverage.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/unused_statements.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"

#include "backends/p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/lib/final_state.h"
#include "backends/p4tools/testgen/lib/test_spec.h"
#include "backends/p4tools/testgen/lib/tf.h"
#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

class TestBackEnd {
 protected:
    /// ProgramInfo is used to access some target specific information for test generation.
    const ProgramInfo& programInfo;

    /// Writes the tests out to a file.
    TF* testWriter = nullptr;

    /// Pointer to the symbolic executor.
    /// TODO: Remove this.
    ExplorationStrategy& symbex;

    /// Test maximum number of tests that are to be produced.
    int maxTests;

    /// Set of all statements,tables,selects in the input P4 program.
    IR::StatementList statementsList;

    /// The current test count. If it exceeds @var maxTests, the symbolic executor will stop.
    int testCount = 0;

    explicit TestBackEnd(const ProgramInfo& programInfo, ExplorationStrategy& symbex)
        : programInfo(programInfo), symbex(symbex), maxTests(TestgenOptions::get().maxTests) {
        // If we select a specific branch, the number of tests should be 1.
        if (!TestgenOptions::get().selectedBranches.empty()) {
            maxTests = 1;
        }
        programInfo.program->apply(UnusedStatements::CollectStatements(statementsList));
    }

 public:
    TestBackEnd(const TestBackEnd&) = default;

    TestBackEnd(TestBackEnd&&) = default;

    TestBackEnd& operator=(const TestBackEnd&) = delete;

    TestBackEnd& operator=(TestBackEnd&&) = delete;

    virtual ~TestBackEnd() = default;

    struct TestInfo {
        /// The concrete value of the input packet.
        /// This is a slice of the program packet according to packetSizeInInt.
        const IR::Constant* inputPacket;

        /// The input port of the packet.
        int inputPort;

        /// The concrete value of the output packet as modified by the packet.
        const IR::Constant* outputPacket;

        /// The output port of the packet.
        int outputPort;

        /// The taint mask.
        const IR::Constant* packetTaintMask;

        /// The traces that have been collected during execution of this particular test path.
        const std::vector<gsl::not_null<const TraceEvent*>> programTraces;

        /// Indicates whether the packet is dropped.
        bool packetIsDropped = false;
    };

    /// @returns the test specification which is consumed by the test back ends.
    virtual const TestSpec* createTestSpec(const ExecutionState* executionState,
                                           const Model* completedModel,
                                           const TestInfo& testInfo) = 0;

    /// Prints information about this particular test path.
    /// @returns false if the test generation is to be aborted (for example when the port is
    /// tainted.)
    virtual bool printTestInfo(const ExecutionState* executionState, const TestInfo& testInfo,
                               int testCount, const IR::Expression* outputPortExpr);

    /// @returns a new modules with all concolic variables in the program resolved.
    const Model* computeConcolicVariables(const ExecutionState* executionState,
                                          const Model* completedModel, Z3Solver* solver,
                                          const IR::Expression* outputPacketExpr,
                                          const IR::Expression* outputPortExpr) const;

    /// @returns a TestInfo objects, which contains information about the input/output ports, the
    /// taint mask, the packet sizes, etc...
    virtual TestInfo produceTestInfo(
        const ExecutionState* executionState, const Model* completedModel,
        const IR::Expression* outputPacketExpr, const IR::Expression* outputPortExpr,
        const std::vector<gsl::not_null<const TraceEvent*>>* programTraces);

    /// The callback that is executed by the symbolic executor.
    virtual bool run(const FinalState& state);

    /// Print out some performance numbers if logging feature "performance" is enabled.
    static void printPerformanceReport();
};

}  // namespace P4Testgen
}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_LIB_TEST_BACKEND_H_ */
