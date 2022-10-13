#include "backends/p4tools/testgen/lib/test_backend.h"

#include <iostream>
#include <map>
#include <utility>

#include <boost/optional/optional.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/timer.h"
#include "backends/p4tools/common/lib/util.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/null.h"

#include "backends/p4tools/testgen/lib/concolic.h"
#include "backends/p4tools/testgen/lib/logging.h"
#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

const Model* TestBackEnd::computeConcolicVariables(const ExecutionState* executionState,
                                                   const Model* completedModel, Z3Solver* solver,
                                                   const IR::Expression* outputPacketExpr,
                                                   const IR::Expression* outputPortExpr) const {
    // Execute concolic functions that may occur in the output packet, the output port,
    // or any path conditions.
    auto concolicResolver =
        ConcolicResolver(completedModel, *executionState, programInfo.getConcolicMethodImpls());

    outputPacketExpr->apply(concolicResolver);
    outputPortExpr->apply(concolicResolver);
    for (const auto* assert : executionState->getPathConstraint()) {
        CHECK_NULL(assert);
        assert->apply(concolicResolver);
    }
    const auto* resolvedConcolicVariables = concolicResolver.getResolvedConcolicVariables();

    // If we resolved concolic variables and substitute them, check the solver again under
    // the new constraints.
    if (!resolvedConcolicVariables->empty()) {
        std::vector<const Constraint*> asserts = executionState->getPathConstraint();

        for (const auto& resolvedConcolicVariable : *resolvedConcolicVariables) {
            const auto& concolicVariable = resolvedConcolicVariable.first;
            const auto* concolicAssignment = resolvedConcolicVariable.second;
            const IR::Expression* pathConstraint = nullptr;
            // We need to differentiate between state variables and expressions here.
            auto concolicType = concolicVariable.which();
            if (concolicType == 0) {
                pathConstraint = new IR::Equ(boost::get<const StateVariable>(concolicVariable),
                                             concolicAssignment);
            } else if (concolicType == 1) {
                pathConstraint = new IR::Equ(boost::get<const IR::Expression*>(concolicVariable),
                                             concolicAssignment);
            }
            CHECK_NULL(pathConstraint);
            pathConstraint = executionState->getSymbolicEnv().subst(pathConstraint);
            pathConstraint = IRUtils::optimizeExpression(pathConstraint);
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
        const auto* concolicFinalState = new FinalState(solver, *executionState);
        completedModel = concolicFinalState->getCompletedModel();
    }
    return completedModel;
}

bool TestBackEnd::run(const FinalState& state) {
    {
        // Evaluate the model and extract the input and output packets.
        const auto* executionState = state.getExecutionState();
        const auto* outputPacketExpr = executionState->getPacketBuffer();
        const auto* completedModel = state.getCompletedModel();
        const auto* outputPortExpr = executionState->get(programInfo.getTargetOutputPortVar());
        const auto& allStatements = programInfo.getAllStatements();
        const Coverage::CoverageSet& visitedStatements = symbex.getVisitedStatements();

        auto* solver = state.getSolver()->to<Z3Solver>();
        CHECK_NULL(solver);

        bool abort = false;
        const auto* concolicModel = computeConcolicVariables(executionState, completedModel, solver,
                                                             outputPacketExpr, outputPortExpr);
        if (concolicModel == nullptr) {
            testCount++;
            Coverage::coverageReportFinal(allStatements, visitedStatements);
            printPerformanceReport();
            return testCount > maxTests - 1;
        }
        completedModel = concolicModel;

        const auto* programTraces = state.getTraces();
        auto testInfo = produceTestInfo(executionState, completedModel, outputPacketExpr,
                                        outputPortExpr, programTraces);

        // Add a list of tracked branches to the test output, too.
        std::stringstream selectedBranches;
        if (TestgenOptions::get().trackBranches) {
            symbex.printCurrentTraceAndBranches(selectedBranches);
        }

        abort = printTestInfo(executionState, testInfo, testCount, outputPortExpr);
        if (abort) {
            testCount++;
            Coverage::coverageReportFinal(allStatements, visitedStatements);
            printPerformanceReport();
            return testCount > maxTests - 1;
        }

        const auto* testSpec = createTestSpec(executionState, completedModel, testInfo);

        float coverage = static_cast<float>(visitedStatements.size()) / allStatements.size();
        printFeature("test_info", 4,
                     "============ Test %1%: Statements covered: %2% (%3%/%4%) ============",
                     testCount, coverage, visitedStatements.size(), allStatements.size());
        Coverage::logCoverage(allStatements, visitedStatements, executionState->getVisited());

        // Output the test.
        withTimer("backend",
                  [&] { testWriter->outputTest(testSpec, selectedBranches, testCount, coverage); });

        printTraces("============ End Test %1% ============\n", testCount);
        Coverage::coverageReportFinal(allStatements, visitedStatements);
        if (TestgenOptions::get().countOfSourceTests > 0) {
            auto unusedStmtList = UnusedStatements::getUnusedStatements(
                executionState->getTrace(), statementsList, visitedStatements);
            if (unusedStmtList.size() == 0) {
                printTraces("============ Dead code not found ============\n");
            } else {
                UnusedStatements::generateTests(
                    unusedStmtList, TestgenOptions::get().countOfSourceTests, testCount);
            }
            printTraces("============ End Source Tests Generation ============\n");
        }
        testCount++;
        printPerformanceReport();
        return testCount > maxTests - 1;
    }
}

TestBackEnd::TestInfo TestBackEnd::produceTestInfo(
    const ExecutionState* executionState, const Model* completedModel,
    const IR::Expression* outputPacketExpr, const IR::Expression* outputPortExpr,
    const std::vector<gsl::not_null<const TraceEvent*>>* programTraces) {
    // Evaluate all the important expressions necessary for program execution by using the
    // completed model.
    int calculatedPacketSize = IRUtils::getIntFromLiteral(
        completedModel->evaluate(ExecutionState::getInputPacketSizeVar()));
    const auto* inputPacketExpr = executionState->getInputPacket();
    // The payload fills the space between the minimum input size needed and the symbolically
    // calculated packet size.
    int payloadSize = calculatedPacketSize - inputPacketExpr->type->width_bits();
    if (payloadSize > 0) {
        const auto* payloadType = IRUtils::getBitType(payloadSize);
        const auto* payloadExpr =
            completedModel->get(ExecutionState::getPayloadLabel(payloadType), false);
        if (payloadExpr == nullptr) {
            payloadExpr = IRUtils::getRandConstantForType(payloadType);
        }
        BUG_CHECK(payloadExpr->type->width_bits() == payloadSize,
                  "The width (%1%) of the payload expression should match the calculated payload "
                  "size %2%.",
                  payloadExpr->type->width_bits(), payloadSize);
        inputPacketExpr =
            new IR::Concat(IRUtils::getBitType(calculatedPacketSize), inputPacketExpr, payloadExpr);
        outputPacketExpr =
            new IR::Concat(IRUtils::getBitType(outputPacketExpr->type->width_bits() + payloadSize),
                           outputPacketExpr, payloadExpr);
    }
    const auto* inputPacket = completedModel->evaluate(inputPacketExpr);
    const auto* outputPacket = completedModel->evaluate(outputPacketExpr);
    const auto* inputPort = completedModel->evaluate(programInfo.getTargetInputPortVar());

    const auto* outputPortVar = completedModel->evaluate(outputPortExpr);
    // Build the taint mask by dissecting the program packet variable
    const auto* evalMask = Taint::buildTaintMask(executionState->getSymbolicEnv().getInternalMap(),
                                                 completedModel, outputPacketExpr);

    // Get the input/output port integers.
    auto inputPortInt = IRUtils::getIntFromLiteral(inputPort);
    auto outputPortInt = IRUtils::getIntFromLiteral(outputPortVar);

    return {inputPacket->checkedTo<IR::Constant>(),   inputPortInt,
            outputPacket->checkedTo<IR::Constant>(),  outputPortInt,
            evalMask->checkedTo<IR::Constant>(),      *programTraces,
            executionState->getProperty<bool>("drop")};
}

bool TestBackEnd::printTestInfo(const ExecutionState* executionState, const TestInfo& testInfo,
                                int testCount, const IR::Expression* outputPortExpr) {
    // Print all the important variables and properties of this test.
    printTraces("============ Program trace for Test %1% ============\n", testCount);
    for (const auto& event : testInfo.programTraces) {
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

void TestBackEnd::printPerformanceReport() {
    printFeature("performance", 4, "============ Timers ============");
    for (const auto& c : getTimers()) {
        if (c.timerName.empty()) {
            printFeature("performance", 4, "Total: %i ms", c.milliseconds);
        } else {
            printFeature("performance", 4, "%s: %i ms (%0.2f %% of parent)", c.timerName,
                         c.milliseconds, c.relativeToParent * 100);
        }
    }
}

}  // namespace P4Testgen

}  // namespace P4Tools
