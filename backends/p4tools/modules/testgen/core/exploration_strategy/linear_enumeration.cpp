#include "backends/p4tools/modules/testgen/core/exploration_strategy/linear_enumeration.h"

#include <vector>

#include <boost/none.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/error.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

void LinearEnumeration::run(const Callback& callback) {
    // Loop until we reach terminate, or until there are no more
    // branches to produce tests.
    while (true) {
        try {
            if (exploredBranches.empty()) {
                return;
            }
            // Select the branch in the vector to produce a test.
            auto idx = selectBranch(exploredBranches);
            auto branch = exploredBranches.at(idx);
            // Erase the element once it produces a test.
            exploredBranches.erase(exploredBranches.begin() + idx);

            // Retrieve the next state (which is guaranteed to be a terminal state)
            // from the branch and invoke handleTerminalState to produce a test.
            ExecutionState* branchState = branch.nextState;
            bool terminate = handleTerminalState(callback, *branchState);
            // This flag indicates we reached maxTests.
            if (terminate) {
                return;
            }
        } catch (TestgenUnimplemented& e) {
            // If strict is enabled, bubble the exception up.
            if (TestgenOptions::get().strict) {
                throw;
            }
            // Otherwise we try to roll back as we typically do.
            ::warning("Path encountered unimplemented feature. Message: %1%\n", e.what());
        }
    }
}

LinearEnumeration::LinearEnumeration(AbstractSolver& solver, const ProgramInfo& programInfo,
                                     uint64_t maxBound)
    : ExplorationStrategy(solver, programInfo), maxBound(maxBound) {
    // The constructor populates the initial vector of branches holding a terminal state.
    // It fill the vector with a recursive call to mapBranch and stops at maxBound.
    StepResult initialSuccessors = step(*executionState);
    // Populates exploredBranches from the initial set of branches.
    for (auto branch : *initialSuccessors) {
        mapBranch(branch);
    }
}

void LinearEnumeration::mapBranch(Branch& branch) {
    // Ensure we don't explore more than maxBound.
    if (exploredBranches.size() >= maxBound) {
        return;
    }

    // Do not bother invoking the solver for a trivial case.
    // In either case (true or false), we do not need to add the assertion and check.
    if (const auto* boolLiteral = branch.constraint->to<IR::BoolLiteral>()) {
        if (!boolLiteral->value) {
            return;
        }
    }

    // Check the consistency of the path constraints asserted so far.
    auto solverResult = solver.checkSat(branch.nextState->getPathConstraint());
    if (solverResult == boost::none) {
        ::warning("Solver timed out");
    }
    if (solverResult == boost::none || !solverResult.get()) {
        // Solver timed out or path constraints were not satisfiable. Need to choose a
        // different branch.
        return;
    }

    // Get branch's next state, if it's terminal, save it in exploredBranches.
    ExecutionState* state = branch.nextState;
    if (state != nullptr) {
        if (state->isTerminal()) {
            // We put the state back so it can be used by handleTerminalState.
            branch.nextState = state;
            exploredBranches.push_back(branch);
        } else {
            // If the state is not terminal, take a step and
            // keep collecting branches.
            auto* successors = step(*state);

            // If the list of successors is not empty, invoke mapBranch recursively.
            if (!successors->empty()) {
                for (auto successorBranch : *successors) {
                    // Recursively invoking mapBranch.
                    mapBranch(successorBranch);
                }
            }
        }
    }
}

}  // namespace P4Testgen

}  // namespace P4Tools
