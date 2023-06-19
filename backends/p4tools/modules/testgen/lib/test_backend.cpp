#include "backends/p4tools/modules/testgen/lib/test_backend.h"

#include <iostream>
#include <optional>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/log.h"
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
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/lib/tf.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

bool TestBackEnd::run(const FinalState &state) {
    {
        // Evaluate the model and extract the input and output packets.
        const auto *executionState = state.getExecutionState();
        const auto *outputPacketExpr = executionState->getPacketBuffer();
        const auto *outputPortExpr = executionState->get(programInfo.getTargetOutputPortVar());
        const auto &coverableNodes = programInfo.getCoverableNodes();
        const auto *programTraces = state.getTraces();

        // Don't increase the test count if --with-output-packet is enabled and we don't
        // produce a test with an output packet.
        if (TestgenOptions::get().withOutputPacket) {
            auto outputPacketSize = executionState->getPacketBufferSize();
            bool packetIsDropped = executionState->getProperty<bool>("drop");
            if (outputPacketSize <= 0 || packetIsDropped) {
                return needsToTerminate(testCount);
            }
        }

        // If assertion mode is active, ignore any test that does not trigger an assertion.
        if (TestgenOptions::get().assertionModeEnabled) {
            if (!executionState->getProperty<bool>("assertionTriggered")) {
                return needsToTerminate(testCount);
            }
            printFeature("test_info", 4,
                         "AssertionMode: Found an input that triggers an assertion.");
        }

        bool abort = false;

        // Execute concolic functions that may occur in the output packet, the output port,
        // or any path conditions.
        auto concolicResolver = ConcolicResolver(state.getFinalModel(), *executionState,
                                                 *programInfo.getConcolicMethodImpls());

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
            printPerformanceReport(false);
            return needsToTerminate(testCount);
        }
        auto replacedState = concolicOptState.value().get();
        executionState = replacedState.getExecutionState();
        outputPacketExpr = executionState->getPacketBuffer();
        const auto &finalModel = replacedState.getFinalModel();
        outputPortExpr = executionState->get(programInfo.getTargetOutputPortVar());

        auto testInfo = produceTestInfo(executionState, &finalModel, outputPacketExpr,
                                        outputPortExpr, programTraces);

        // Add a list of tracked branches to the test output, too.
        std::stringstream selectedBranches;
        if (TestgenOptions::get().trackBranches) {
            symbex.printCurrentTraceAndBranches(selectedBranches);
        }

        abort = printTestInfo(executionState, testInfo, outputPortExpr);
        if (abort) {
            testCount++;
            printPerformanceReport(false);
            return needsToTerminate(testCount);
        }
        const auto *testSpec = createTestSpec(executionState, &finalModel, testInfo);

        // Commit an update to the visited statements.
        // Only do this once we are sure we are generating a test.
        symbex.updateVisitedNodes(replacedState.getVisited());
        const P4::Coverage::CoverageSet &visitedNodes = symbex.getVisitedNodes();
        float coverage = NAN;
        if (coverableNodes.empty()) {
            printFeature("test_info", 4, "============ Test %1% ============", testCount);
        } else {
            coverage =
                static_cast<float>(visitedNodes.size()) / static_cast<float>(coverableNodes.size());
            printFeature("test_info", 4,
                         "============ Test %1%: Nodes covered: %2% (%3%/%4%) ============",
                         testCount, coverage, visitedNodes.size(), coverableNodes.size());
            P4::Coverage::logCoverage(coverableNodes, visitedNodes, executionState->getVisited());
        }

        // Output the test.
        Util::withTimer("backend", [this, &testSpec, &selectedBranches, &coverage] {
            testWriter->outputTest(testSpec, selectedBranches, testCount, coverage);
        });

        printTraces("============ End Test %1% ============\n", testCount);
        testCount++;
        P4::Coverage::printCoverageReport(coverableNodes, visitedNodes);
        printPerformanceReport(false);

        // If MAX_STATEMENT_COVERAGE is enabled, terminate early if we hit max coverage already.
        if (TestgenOptions::get().stopMetric == "MAX_STATEMENT_COVERAGE" && coverage == 1.0) {
            return true;
        }
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
        finalModel->evaluate(executionState->get(programInfo.getTargetInputPortVar()), true);

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
    printTraces(formatHexExpr(testInfo.inputPacket, false, true, false));
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
    printTraces(formatHexExpr(testInfo.outputPacket, false, true, false));
    printTraces("=======================================");
    printTraces("Output packet size: %1% ", outputPacketSize);
    printTraces("=======================================");
    printTraces("============ Output mask Test %1% ============", testCount);
    printTraces(formatHexExpr(testInfo.packetTaintMask, false, true, false));
    printTraces("=======================================");
    printTraces("Output port: %1%\n", testInfo.outputPort);
    printTraces("=======================================");

    return false;
}

void TestBackEnd::printPerformanceReport(bool write) const {
    testWriter->printPerformanceReport(write);
}

int64_t TestBackEnd::getTestCount() const { return testCount; }

}  // namespace P4Tools::P4Testgen
