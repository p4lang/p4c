#include "backends/p4tools/modules/testgen/lib/test_backend.h"

#include <iostream>
#include <map>
#include <set>
#include <utility>

#include <boost/format.hpp>
#include <boost/optional/optional.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/irutils.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/timer.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/final_state.h"
#include "backends/p4tools/modules/testgen/lib/logging.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/lib/tf.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

const Model *TestBackEnd::computeConcolicVariables(const ExecutionState *executionState,
                                                   const Model *completedModel, Z3Solver *solver,
                                                   const IR::Expression *outputPacketExpr,
                                                   const IR::Expression *outputPortExpr) const {
    // Execute concolic functions that may occur in the output packet, the output port,
    // or any path conditions.
    auto concolicResolver =
        ConcolicResolver(completedModel, *executionState, programInfo.getConcolicMethodImpls());

    outputPacketExpr->apply(concolicResolver);
    outputPortExpr->apply(concolicResolver);
    for (const auto *assert : executionState->getPathConstraint()) {
        CHECK_NULL(assert);
        assert->apply(concolicResolver);
    }
    const auto *resolvedConcolicVariables = concolicResolver.getResolvedConcolicVariables();

    // If we resolved concolic variables and substitute them, check the solver again under
    // the new constraints.
    if (!resolvedConcolicVariables->empty()) {
        std::vector<const Constraint *> asserts = executionState->getPathConstraint();

        for (const auto &resolvedConcolicVariable : *resolvedConcolicVariables) {
            const auto &concolicVariable = resolvedConcolicVariable.first;
            const auto *concolicAssignment = resolvedConcolicVariable.second;
            const IR::Expression *pathConstraint = nullptr;
            // We need to differentiate between state variables and expressions here.
            auto concolicType = concolicVariable.which();
            if (concolicType == 0) {
                pathConstraint = new IR::Equ(boost::get<const StateVariable>(concolicVariable),
                                             concolicAssignment);
            } else if (concolicType == 1) {
                pathConstraint = new IR::Equ(boost::get<const IR::Expression *>(concolicVariable),
                                             concolicAssignment);
            }
            CHECK_NULL(pathConstraint);
            pathConstraint = executionState->getSymbolicEnv().subst(pathConstraint);
            pathConstraint = P4::optimizeExpression(pathConstraint);
            asserts.push_back(pathConstraint);
        }
        auto solverResult = solver->checkSat(asserts);
        if (!solverResult) {
            ::warning("Timed out trying to solve this concolic execution path.");
            return nullptr;
        }

        if (!*solverResult) {
            ::warning("Concolic constraints for this path are unsatisfiable.");
            return nullptr;
        }
        const auto *concolicFinalState = new FinalState(solver, *executionState);
        completedModel = concolicFinalState->getCompletedModel();
    }
    return completedModel;
}

bool TestBackEnd::run(const FinalState &state) {
    {
        // Evaluate the model and extract the input and output packets.
        const auto *executionState = state.getExecutionState();
        const auto *outputPacketExpr = executionState->getPacketBuffer();
        const auto *completedModel = state.getCompletedModel();
        const auto *outputPortExpr = executionState->get(programInfo.getTargetOutputPortVar());
        const auto &allStatements = programInfo.getAllStatements();

        auto *solver = state.getSolver()->to<Z3Solver>();
        CHECK_NULL(solver);

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
        const auto *concolicModel = computeConcolicVariables(executionState, completedModel, solver,
                                                             outputPacketExpr, outputPortExpr);
        if (concolicModel == nullptr) {
            testCount++;
            printPerformanceReport();
            return needsToTerminate(testCount);
        }
        completedModel = concolicModel;

        const auto *programTraces = state.getTraces();
        auto testInfo = produceTestInfo(executionState, completedModel, outputPacketExpr,
                                        outputPortExpr, programTraces);

        // Add a list of tracked branches to the test output, too.
        std::stringstream selectedBranches;
        if (TestgenOptions::get().trackBranches) {
            symbex.printCurrentTraceAndBranches(selectedBranches);
        }

        abort = printTestInfo(executionState, testInfo, outputPortExpr);
        if (abort) {
            testCount++;
            printPerformanceReport();
            return needsToTerminate(testCount);
        }
        const auto *testSpec = createTestSpec(executionState, completedModel, testInfo);

        // Commit an update to the visited statements.
        // Only do this once we are sure we are generating a test.
        symbex.updateVisitedStatements(state.getVisited());
        const P4::Coverage::CoverageSet &visitedStatements = symbex.getVisitedStatements();
        auto coverage =
            static_cast<float>(visitedStatements.size()) / static_cast<float>(allStatements.size());
        printFeature("test_info", 4,
                     "============ Test %1%: Statements covered: %2% (%3%/%4%) ============",
                     testCount, coverage, visitedStatements.size(), allStatements.size());
        P4::Coverage::logCoverage(allStatements, visitedStatements, executionState->getVisited());

        // Output the test.
        Util::withTimer("backend", [this, &testSpec, &selectedBranches, &coverage] {
            testWriter->outputTest(testSpec, selectedBranches, testCount, coverage);
        });

        printTraces("============ End Test %1% ============\n", testCount);
        testCount++;
        P4::Coverage::coverageReportFinal(allStatements, visitedStatements);
        printPerformanceReport();

        // If MAX_STATEMENT_COVERAGE is enabled, terminate early if we hit max coverage already.
        if (TestgenOptions::get().stopMetric == "MAX_STATEMENT_COVERAGE" && coverage == 1.0) {
            return true;
        }
        return needsToTerminate(testCount);
    }
}

TestBackEnd::TestInfo TestBackEnd::produceTestInfo(
    const ExecutionState *executionState, const Model *completedModel,
    const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
    const std::vector<gsl::not_null<const TraceEvent *>> *programTraces) {
    // Evaluate all the important expressions necessary for program execution by using the
    // completed model.
    int calculatedPacketSize =
        IR::getIntFromLiteral(completedModel->evaluate(ExecutionState::getInputPacketSizeVar()));
    const auto *inputPacketExpr = executionState->getInputPacket();
    // The payload fills the space between the minimum input size needed and the symbolically
    // calculated packet size.
    int payloadSize = calculatedPacketSize - inputPacketExpr->type->width_bits();
    if (payloadSize > 0) {
        const auto *payloadType = IR::getBitType(payloadSize);
        const auto *payloadExpr =
            completedModel->get(ExecutionState::getPayloadLabel(payloadType), false);
        if (payloadExpr == nullptr) {
            payloadExpr = Utils::getRandConstantForType(payloadType);
        }
        BUG_CHECK(payloadExpr->type->width_bits() == payloadSize,
                  "The width (%1%) of the payload expression should match the calculated payload "
                  "size %2%.",
                  payloadExpr->type->width_bits(), payloadSize);
        inputPacketExpr =
            new IR::Concat(IR::getBitType(calculatedPacketSize), inputPacketExpr, payloadExpr);
        outputPacketExpr =
            new IR::Concat(IR::getBitType(outputPacketExpr->type->width_bits() + payloadSize),
                           outputPacketExpr, payloadExpr);
    }
    const auto *inputPacket = completedModel->evaluate(inputPacketExpr);
    const auto *outputPacket = completedModel->evaluate(outputPacketExpr);
    const auto *inputPort = completedModel->evaluate(programInfo.getTargetInputPortVar());

    const auto *outputPortVar = completedModel->evaluate(outputPortExpr);
    // Build the taint mask by dissecting the program packet variable
    const auto *evalMask = Taint::buildTaintMask(executionState->getSymbolicEnv().getInternalMap(),
                                                 completedModel, outputPacketExpr);

    // Get the input/output port integers.
    auto inputPortInt = IR::getIntFromLiteral(inputPort);
    auto outputPortInt = IR::getIntFromLiteral(outputPortVar);

    return {inputPacket->checkedTo<IR::Constant>(),   inputPortInt,
            outputPacket->checkedTo<IR::Constant>(),  outputPortInt,
            evalMask->checkedTo<IR::Constant>(),      *programTraces,
            executionState->getProperty<bool>("drop")};
}

bool TestBackEnd::printTestInfo(const ExecutionState *executionState, const TestInfo &testInfo,
                                const IR::Expression *outputPortExpr) {
    // Print all the important variables and properties of this test.
    printTraces("============ Program trace for Test %1% ============\n", testCount);
    for (const auto &event : testInfo.programTraces) {
        printTraces("%1%", *event);
    }

    auto inputPacketSize = testInfo.inputPacket->type->width_bits();
    auto outputPacketSize = testInfo.outputPacket->type->width_bits();

    printTraces("=======================================");
    printTraces("============ Input packet for Test %1% ============", testCount);
    printTraces(formatHexExpr(testInfo.inputPacket, false, true, false));
    printTraces("=======================================");
    // We have no control over the test, if the output port is tainted. So we abort.
    if (executionState->hasTaint(outputPortExpr)) {
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

void TestBackEnd::printPerformanceReport() const { testWriter->printPerformanceReport(); }

int64_t TestBackEnd::getTestCount() const { return testCount; }

}  // namespace P4Tools::P4Testgen
