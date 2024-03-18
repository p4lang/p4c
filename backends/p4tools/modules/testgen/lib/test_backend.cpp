#include "backends/p4tools/modules/testgen/lib/test_backend.h"

#include <optional>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/timer.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/final_state.h"
#include "backends/p4tools/modules/testgen/lib/logging.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

TestBackEnd::TestBackEnd(const ProgramInfo &programInfo,
                         const TestBackendConfiguration &testBackendConfiguration,
                         SymbolicExecutor &symbex)
    : programInfo(programInfo),
      testBackendConfiguration(testBackendConfiguration),
      symbex(symbex),
      maxTests(TestgenOptions::get().maxTests) {
    // If we select a specific branch, the number of tests should be 1.
    if (!TestgenOptions::get().selectedBranches.empty()) {
        maxTests = 1;
    }
}

bool TestBackEnd::run(const FinalState &state) {
    {
        // Evaluate the model and extract the input and output packets.
        const auto *executionState = state.getExecutionState();
        const auto *outputPacketExpr = executionState->getPacketBuffer();
        const auto *outputPortExpr = executionState->get(getProgramInfo().getTargetOutputPortVar());
        const auto &coverableNodes = getProgramInfo().getCoverableNodes();
        const auto *programTraces = state.getTraces();
        const auto &testgenOptions = TestgenOptions::get();

        // Don't increase the test count if --output-packet-only is enabled and we don't
        // produce a test with an output packet.
        if (testgenOptions.outputPacketOnly) {
            if (executionState->getPacketBufferSize() <= 0 ||
                executionState->getProperty<bool>("drop")) {
                return needsToTerminate(testCount);
            }
        }

        // Don't increase the test count if --dropped-packet-only is enabled and we produce a test
        // with an output packet.
        if (testgenOptions.droppedPacketOnly) {
            if (!executionState->getProperty<bool>("drop")) {
                return needsToTerminate(testCount);
            }
        }

        // If assertion mode is active, ignore any test that does not trigger an assertion.
        if (testgenOptions.assertionModeEnabled) {
            if (!executionState->getProperty<bool>("assertionTriggered")) {
                return needsToTerminate(testCount);
            }
            printFeature("test_info", 4,
                         "AssertionMode: Found an input that triggers an assertion.");
        }

        // For long-running tests periodically reset the solver state to free up memory.
        if (testCount != 0 && testCount % RESET_THRESHOLD == 0) {
            auto &solver = state.getSolver();
            auto *z3Solver = solver.to<Z3Solver>();
            CHECK_NULL(z3Solver);
            z3Solver->clearMemory();
        }

        bool abort = false;

        // Execute concolic functions that may occur in the output packet, the output port,
        // or any path conditions.
        auto concolicResolver = ConcolicResolver(state.getFinalModel(), *executionState,
                                                 *getProgramInfo().getConcolicMethodImpls());

        outputPacketExpr->apply(concolicResolver);
        outputPortExpr->apply(concolicResolver);
        for (const auto *assert : executionState->getPathConstraint()) {
            CHECK_NULL(assert);
            assert->apply(concolicResolver);
        }
        const ConcolicVariableMap *resolvedConcolicVariables =
            concolicResolver.getResolvedConcolicVariables();
        // If we resolved concolic variables and substitute them, check the solver again under
        // the new constraints.
        auto concolicOptState = state.computeConcolicState(*resolvedConcolicVariables);
        if (!concolicOptState.has_value()) {
            testCount++;
            return needsToTerminate(testCount);
        }
        auto replacedState = concolicOptState.value().get();
        executionState = replacedState.getExecutionState();
        outputPacketExpr = executionState->getPacketBuffer();
        const auto &finalModel = replacedState.getFinalModel();
        outputPortExpr = executionState->get(getProgramInfo().getTargetOutputPortVar());

        auto testInfo = produceTestInfo(executionState, &finalModel, outputPacketExpr,
                                        outputPortExpr, programTraces);

        // Add a list of tracked branches to the test output, too.
        std::stringstream selectedBranches;
        if (testgenOptions.trackBranches) {
            symbex.printCurrentTraceAndBranches(selectedBranches, *executionState);
        }

        abort = printTestInfo(executionState, testInfo, outputPortExpr);
        if (abort) {
            testCount++;
            return needsToTerminate(testCount);
        }
        const auto *testSpec = createTestSpec(executionState, &finalModel, testInfo);

        // Commit an update to the visited nodes.
        // Only do this once we are sure we are generating a test.
        auto hasUpdated = symbex.updateVisitedNodes(replacedState.getVisited());

        // Skip test case generation if the --only-covering-tests is enabled and we do not increase
        // coverage.
        if (!coverableNodes.empty() && testgenOptions.coverageOptions.onlyCoveringTests &&
            !hasUpdated) {
            return needsToTerminate(testCount);
        }

        testCount++;
        const P4::Coverage::CoverageSet &visitedNodes = symbex.getVisitedNodes();
        if (!testgenOptions.hasCoverageTracking) {
            printFeature("test_info", 4, "============ Test %1% ============", testCount);
        } else if (coverableNodes.empty()) {
            printFeature("test_info", 4,
                         "============ Test %1%: No coverable nodes ============", testCount);
            // All 0 nodes covered.
            coverage = 1.0;
        } else {
            coverage =
                static_cast<float>(visitedNodes.size()) / static_cast<float>(coverableNodes.size());
            printFeature("test_info", 4,
                         "============ Test %1%: Nodes covered: %2% (%3%/%4%) ============",
                         testCount, coverage, visitedNodes.size(), coverableNodes.size());
            P4::Coverage::logCoverage(coverableNodes, visitedNodes, executionState->getVisited());
        }

        // Output the test.
        Util::withTimer("backend", [this, &testSpec, &selectedBranches] {
            if (testWriter->isInFileMode()) {
                testWriter->writeTestToFile(testSpec, selectedBranches, testCount, coverage);
            } else {
                auto testOpt =
                    testWriter->produceTest(testSpec, selectedBranches, testCount, coverage);
                if (!testOpt.has_value()) {
                    BUG("Failed to produce test.");
                }
                tests.push_back(testOpt.value());
            }
        });

        printTraces("============ End Test %1% ============\n", testCount);
        P4::Coverage::printCoverageReport(coverableNodes, visitedNodes);

        // If MAX_NODE_COVERAGE is enabled, terminate early if we hit max node coverage already.
        if (testgenOptions.stopMetric == "MAX_NODE_COVERAGE" && coverage == 1.0) {
            return true;
        }
#ifdef P4TESTGEN_PRINT_PERFORMANCE_PER_TEST
        printPerformanceReport(std::nullopt);
#endif
        return needsToTerminate(testCount);
    }
}

TestBackEnd::TestInfo TestBackEnd::produceTestInfo(
    const ExecutionState *executionState, const Model *finalModel,
    const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
    const std::vector<std::reference_wrapper<const TraceEvent>> *programTraces) {
    // Evaluate all the important expressions necessary for program execution by using the
    // final model.
    int calculatedPacketSize =
        IR::getIntFromLiteral(finalModel->evaluate(ExecutionState::getInputPacketSizeVar(), true));
    const auto *inputPacketExpr = executionState->getInputPacket();
    // The payload fills the space between the minimum input size needed and the symbolically
    // calculated packet size.
    const auto *payloadExpr = finalModel->get(&PacketVars::PAYLOAD_SYMBOL, false);
    if (payloadExpr != nullptr) {
        inputPacketExpr =
            new IR::Concat(IR::getBitType(calculatedPacketSize), inputPacketExpr, payloadExpr);
        outputPacketExpr = new IR::Concat(
            IR::getBitType(outputPacketExpr->type->width_bits() + payloadExpr->type->width_bits()),
            outputPacketExpr, payloadExpr);
    }
    const auto *inputPacket = finalModel->evaluate(inputPacketExpr, true);
    const auto *outputPacket = finalModel->evaluate(outputPacketExpr, true);
    const auto *inputPort =
        finalModel->evaluate(executionState->get(getProgramInfo().getTargetInputPortVar()), true);

    const auto *outputPortVar = finalModel->evaluate(outputPortExpr, true);
    // Build the taint mask by dissecting the program packet variable
    const auto *evalMask = Taint::buildTaintMask(finalModel, outputPacketExpr);

    // Get the input/output port integers.
    auto inputPortInt = IR::getIntFromLiteral(inputPort);
    auto outputPortInt = IR::getIntFromLiteral(outputPortVar);

    return {inputPacket->checkedTo<IR::Constant>(),   inputPortInt,
            outputPacket->checkedTo<IR::Constant>(),  outputPortInt,
            evalMask->checkedTo<IR::Constant>(),      *programTraces,
            executionState->getProperty<bool>("drop")};
}

bool TestBackEnd::printTestInfo(const ExecutionState * /*executionState*/, const TestInfo &testInfo,
                                const IR::Expression *outputPortExpr) {
    // Print all the important variables and properties of this test.
    printTraces("============ Program trace for Test %1% ============\n", testCount);
    for (const auto &event : testInfo.programTraces) {
        printTraces("%1%", event);
    }

    auto inputPacketSize = testInfo.inputPacket->type->width_bits();
    auto outputPacketSize = testInfo.outputPacket->type->width_bits();

    printTraces("=======================================");
    printTraces("============ Input packet for Test %1% ============", testCount);
    printTraces(formatHexExpr(testInfo.inputPacket, {false, true, false}));
    printTraces("=======================================");
    // We have no control over the test, if the output port is tainted. So we abort.
    if (Taint::hasTaint(outputPortExpr)) {
        printFeature(
            "test_info", 4,
            "============ Test %1%: Output port tainted - Aborting Test ============", testCount);
        return true;
    }
    printTraces("Input packet size: %1%", inputPacketSize);
    printTraces("============ Ports for Test %1% ============", testCount);
    printTraces("Input port: %1%", testInfo.inputPort);
    printTraces("=======================================");

    if (testInfo.packetIsDropped) {
        printTraces("============ Output packet dropped for Test %1% ============", testCount);
        return false;
    }

    BUG_CHECK(outputPacketSize >= 0, "Invalid out packet size (%1% bits) calculated!",
              outputPacketSize);
    printTraces("============ Output packet for Test %1% ============", testCount);
    printTraces(formatHexExpr(testInfo.outputPacket, {false, true, false}));
    printTraces("=======================================");
    printTraces("Output packet size: %1% ", outputPacketSize);
    printTraces("=======================================");
    printTraces("============ Output mask Test %1% ============", testCount);
    printTraces(formatHexExpr(testInfo.packetTaintMask, {false, true, false}));
    printTraces("=======================================");
    printTraces("Output port: %1%\n", testInfo.outputPort);
    printTraces("=======================================");

    return false;
}

int64_t TestBackEnd::getTestCount() const { return testCount; }

float TestBackEnd::getCoverage() const { return coverage; }

const ProgramInfo &TestBackEnd::getProgramInfo() const { return programInfo; }

const TestBackendConfiguration &TestBackEnd::getTestBackendConfiguration() const {
    return testBackendConfiguration;
}

bool TestBackEnd::needsToTerminate(int64_t testCount) const {
    // If maxTests is 0, we never "need" to terminate because we want to produce as many tests as
    // possible.
    return maxTests != 0 && testCount >= maxTests;
}
}  // namespace P4Tools::P4Testgen
