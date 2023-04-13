#include "backends/p4tools/modules/testgen/core/symbolic_executor/greedy_stmt_cov.h"

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "lib/error.h"
#include "lib/timer.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

GreedyStmtSelection::GreedyStmtSelection(AbstractSolver &solver, const ProgramInfo &programInfo)
    : SymbolicExecutor(solver, programInfo) {}

std::optional<SymbolicExecutor::Branch> GreedyStmtSelection::popPotentialBranch(
    const P4::Coverage::CoverageSet &coveredStatements,
    std::vector<SymbolicExecutor::Branch> &candidateBranches) {
    for (size_t idx = 0; idx < candidateBranches.size(); ++idx) {
        auto branch = candidateBranches.at(idx);
        // First check all the potential set of statements we can cover by looking ahead.
        for (const auto &stmt : branch.potentialStatements) {
            if (coveredStatements.count(stmt) == 0U) {
                candidateBranches[idx] = candidateBranches.back();
                candidateBranches.pop_back();
                return branch;
            }
        }
        // If we did not find anything, check whether this state covers any new statements
        // already.
        for (const auto &stmt : branch.nextState.get().getVisited()) {
            if (coveredStatements.count(stmt) == 0U) {
                candidateBranches[idx] = candidateBranches.back();
                candidateBranches.pop_back();
                return branch;
            }
        }
    }
    return std::nullopt;
}

bool GreedyStmtSelection::pickSuccessor(StepResult successors) {
    if (successors->empty()) {
        return false;
    }
    // If there is only one successor, choose it and move on.
    if (successors->size() == 1) {
        executionState = successors->at(0).nextState;
        return true;
    }

    stepsWithoutTest++;
    // Only perform a greedy search if we are still producing tests consistently.
    // This guard is necessary to avoid getting caught in parser loops.
    if (stepsWithoutTest < MAX_STEPS_WITHOUT_TEST) {
        // Try to find a branch that covers new statements.
        auto branch = popPotentialBranch(getVisitedStatements(), *successors);
        // If we succeed, pick the branch and add the remainder to the list of
        // potential branches.
        if (branch.has_value()) {
            executionState = branch->nextState;
            potentialBranches.insert(potentialBranches.end(), successors->begin(),
                                     successors->end());
            return true;
        }
    }
    // If we can not cover anything new, pick a branch at random.
    executionState = popRandomBranch(*successors).nextState;
    // Add the remaining tests to the unexplored branches.
    unexploredBranches.insert(unexploredBranches.end(), successors->begin(), successors->end());
    return true;
}

void GreedyStmtSelection::run(const Callback &callback) {
    while (true) {
        try {
            if (executionState.get().isTerminal()) {
                // We've reached the end of the program. Call back and (if desired) end execution.
                bool terminate = handleTerminalState(callback, executionState);
                stepsWithoutTest = 0;
                if (terminate) {
                    return;
                }
            } else {
                // Take a step in the program, choose a branch, and continue execution. If
                // branch selection fails, fall through to the roll-back code below. To help reduce
                // calls into the solver, only guarantee viability of the selected branch if more
                // than one branch was produced.
                // State successors are accompanied by branch constraint which should be evaluated
                // in the state before the step was taken - we copy the current symbolic state.
                StepResult successors = step(executionState);
                auto success = pickSuccessor(successors);
                if (success) {
                    continue;
                }
            }
        } catch (TestgenUnimplemented &e) {
            // If strict is enabled, bubble the exception up.
            if (TestgenOptions::get().strict) {
                throw;
            }
            // Otherwise we try to roll back as we typically do.
            ::warning("Path encountered unimplemented feature. Message: %1%\n", e.what());
        }

        // Roll back to a previous branch and continue execution from there, but if there are no
        // more branches to explore, finish execution. Not all branches are viable, so we loop
        // until either we run out of unexplored branches or we find a viable branch.
        if (potentialBranches.empty() && unexploredBranches.empty()) {
            return;
        }
        // Select a new branch by iterating over all branches
        Util::ScopedTimer chooseBranchtimer("branch_selection");
        auto branch = popPotentialBranch(getVisitedStatements(), potentialBranches);
        if (branch.has_value()) {
            executionState = branch->nextState;
            continue;
        }
        // We did not find a single branch that could cover new state.
        // Add all potential branches to the list of unexplored branches.
        unexploredBranches.insert(unexploredBranches.end(), potentialBranches.begin(),
                                  potentialBranches.end());
        potentialBranches.clear();
        // If we did not find any new statements, fall back to random.
        executionState = popRandomBranch(unexploredBranches).nextState;
    }
}

}  // namespace P4Tools::P4Testgen
