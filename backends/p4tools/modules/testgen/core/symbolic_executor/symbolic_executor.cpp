#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "lib/error.h"
#include "lib/timer.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/final_state.h"
#include "backends/p4tools/modules/testgen/lib/logging.h"

namespace P4Tools::P4Testgen {

SymbolicExecutor::StepResult SymbolicExecutor::step(ExecutionState &state) {
    StepResult successors = nullptr;
    // Use a scope here to measure the time it takes for a step.
    {
        Util::ScopedTimer st("step");
        successors = evaluator.step(state);
    }
    // Remove any successors that are unsatisfiable.
    successors->erase(
        std::remove_if(successors->begin(), successors->end(),
                       [this](const Branch &b) -> bool { return !evaluateBranch(b, solver); }),
        successors->end());
    return successors;
}

void SymbolicExecutor::run(const Callback &callBack) {
    runImpl(callBack, ExecutionState::create(&programInfo.getP4Program()));
}

bool SymbolicExecutor::handleTerminalState(const Callback &callback,
                                           const ExecutionState &terminalState) {
    // Check the solver for satisfiability. If it times out or reports non-satisfiability, issue
    // a warning and continue on a different path.
    auto solverResult = solver.checkSat(terminalState.getPathConstraint());
    if (!solverResult) {
        ::warning("Solver timed out");
        return false;
    }

    if (!*solverResult) {
        ::warning("Path constraints unsatisfiable");
        return false;
    }

    // Get the model from the solver, complete it with respect to the
    // final symbolic environment and trace, use it to evaluate the
    // final execution state, and finally delegate to the callback.
    const FinalState finalState(solver, terminalState);
    return callback(finalState);
}

bool SymbolicExecutor::evaluateBranch(const SymbolicExecutor::Branch &branch,
                                      AbstractSolver &solver) {
    // Do not bother invoking the solver for a trivial case.
    // In either case (true or false), we do not need to add the assertion and check.
    if (const auto *boolLiteral = branch.constraint->to<IR::BoolLiteral>()) {
        return boolLiteral->value;
    }

    // Check the consistency of the path constraints asserted so far.
    auto solverResult = solver.checkSat(branch.nextState.get().getPathConstraint());
    if (solverResult == std::nullopt) {
        ::warning("Solver timed out");
    }
    return solverResult.value_or(false);
}

SymbolicExecutor::Branch SymbolicExecutor::popRandomBranch(
    std::vector<SymbolicExecutor::Branch> &candidateBranches) {
    auto branchIdx = Utils::getRandInt(candidateBranches.size() - 1);
    auto branch = candidateBranches[branchIdx];
    candidateBranches[branchIdx] = candidateBranches.back();
    candidateBranches.pop_back();
    return branch;
}

SymbolicExecutor::SymbolicExecutor(AbstractSolver &solver, const ProgramInfo &programInfo)
    : programInfo(programInfo),
      solver(solver),
      coverableNodes(programInfo.getCoverableNodes()),
      evaluator(solver, programInfo) {
    // If there is no seed provided, do not randomize the solver.
    auto seed = Utils::getCurrentSeed();
    if (seed != std::nullopt) {
        this->solver.seed(*seed);
    }
}

bool SymbolicExecutor::updateVisitedNodes(const P4::Coverage::CoverageSet &newNodes) {
    auto hasUpdated = false;
    for (auto newNode : newNodes) {
        hasUpdated |= visitedNodes.insert(newNode).second;
    }
    return hasUpdated;
}

const P4::Coverage::CoverageSet &SymbolicExecutor::getVisitedNodes() { return visitedNodes; }

void SymbolicExecutor::printCurrentTraceAndBranches(std::ostream &out,
                                                    const ExecutionState &executionState) {
    const auto &branchesList = executionState.getSelectedBranches();
    printTraces("Track branches:");
    out << "Selected " << branchesList.size() << " branches : (";
    printTraces("Selected %1% branches : (", branchesList.size());
    std::stringstream tmpString;
    std::copy(branchesList.begin(), branchesList.end(), std::ostream_iterator<int>(tmpString, ","));
    std::string strBranches = tmpString.str();
    if (!strBranches.empty()) {
        strBranches.erase(strBranches.length() - 1);
    }
    printTraces(" %1% ) \n", strBranches);
    out << strBranches << ")";
}

}  // namespace P4Tools::P4Testgen
