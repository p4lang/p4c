#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <boost/format.hpp>
#include <boost/none.hpp>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/timer.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/p4/optimizeExpressions.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/error.h"
#include "midend/coverage.h"
#include "p4tools/common/core/solver.h"
#include "p4tools/common/lib/formulae.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/final_state.h"
#include "backends/p4tools/modules/testgen/lib/logging.h"

namespace P4Tools {

namespace P4Testgen {

ExplorationStrategy::Branch::Branch(gsl::not_null<ExecutionState*> nextState)
    : constraint(IR::getBoolLiteral(true)), nextState(std::move(nextState)) {}

ExplorationStrategy::Branch::Branch(boost::optional<const Constraint*> c,
                                    const ExecutionState& prevState,
                                    gsl::not_null<ExecutionState*> nextState)
    : constraint(IR::getBoolLiteral(true)), nextState(nextState) {
    if (c) {
        // Evaluate the branch constraint in the current state of symbolic environment.
        // Substitutes all variables to their symbolic value (expression on the program's initial
        // state).
        constraint = prevState.getSymbolicEnv().subst(*c);
        constraint = P4::optimizeExpression(constraint);
        // Append the evaluated and optimized constraint to the next execution state's list of
        // path constraints.
        nextState->pushPathConstraint(constraint);
    }
}

ExplorationStrategy::StepResult ExplorationStrategy::step(ExecutionState& state) {
    ScopedTimer st("step");
    StepResult successors = evaluator.step(state);
    // Assign branch ids to the branches. These integer branch ids are used by track-branches
    // and selected (input) branches features.
    if (successors->size() > 1) {
        for (uint64_t bIdx = 0; bIdx < successors->size(); ++bIdx) {
            auto& succ = (*successors)[bIdx];
            succ.nextState->pushBranchDecision(bIdx + 1);
        }
    }
    return successors;
}

uint64_t ExplorationStrategy::selectBranch(const std::vector<Branch>& branches) {
    // Pick a branch at random.
    return Utils::getRandInt(branches.size() - 1);
}

bool ExplorationStrategy::handleTerminalState(const Callback& callback,
                                              const ExecutionState& terminalState) {
    // We update the set of visitedStatements in every terminal state.
    for (const auto& stmt : terminalState.getVisited()) {
        if (allStatements.count(stmt) != 0U) {
            visitedStatements.insert(stmt);
        }
    }
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
    const FinalState finalState(&solver, terminalState);
    return callback(finalState);
}

ExplorationStrategy::ExplorationStrategy(AbstractSolver& solver, const ProgramInfo& programInfo,
                                         boost::optional<uint32_t> seed)
    : programInfo(programInfo),
      solver(solver),
      allStatements(programInfo.getAllStatements()),
      evaluator(solver, programInfo) {
    // If there is no seed provided, do not randomize the solver.
    if (seed != boost::none) {
        this->solver.seed(*seed);
    }
    executionState = new ExecutionState(programInfo.program);
}

const P4::Coverage::CoverageSet& ExplorationStrategy::getVisitedStatements() {
    return visitedStatements;
}

void ExplorationStrategy::printCurrentTraceAndBranches(std::ostream& out) {
    if (executionState == nullptr) {
        return;
    }
    const auto& branchesList = executionState->getSelectedBranches();
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

}  // namespace P4Testgen

}  // namespace P4Tools
