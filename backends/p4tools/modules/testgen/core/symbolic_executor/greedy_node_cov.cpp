#include "backends/p4tools/modules/testgen/core/symbolic_executor/greedy_node_cov.h"

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <vector>

#include "ir/solver.h"
#include "lib/error.h"
#include "lib/timer.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

GreedyNodeSelection::GreedyNodeSelection(AbstractSolver &solver, const ProgramInfo &programInfo)
    : SymbolicExecutor(solver, programInfo) {}

std::optional<SymbolicExecutor::Branch> GreedyNodeSelection::popPotentialBranch(
    const P4::Coverage::CoverageSet &coveredNodes,
    std::vector<SymbolicExecutor::Branch> &candidateBranches) {
    for (size_t idx = 0; idx < candidateBranches.size(); ++idx) {
        auto branch = candidateBranches.at(idx);
        // First check all the potential set of nodes we can cover by looking ahead.
        for (const auto &stmt : branch.potentialNodes) {
            if (coveredNodes.count(stmt) == 0U) {
                candidateBranches[idx] = candidateBranches.back();
                candidateBranches.pop_back();
                return branch;
            }
        }
        // If we did not find anything, check whether this state covers any new nodes
        // already.
        for (const auto &stmt : branch.nextState.get().getVisited()) {
            if (coveredNodes.count(stmt) == 0U) {
                candidateBranches[idx] = candidateBranches.back();
                candidateBranches.pop_back();
                return branch;
            }
        }
    }
    return std::nullopt;
}

std::optional<ExecutionStateReference> GreedyNodeSelection::pickSuccessor(StepResult successors) {
    if (successors->empty()) {
        return std::nullopt;
    }
    // If there is only one successor, choose it and move on.
    if (successors->size() == 1) {
        return successors->at(0).nextState;
    }

    stepsWithoutTest++;
    // Only perform a greedy search if we are still producing tests consistently.
    // This guard is necessary to avoid getting caught in parser loops.
    if (stepsWithoutTest < MAX_STEPS_WITHOUT_TEST) {
        // Try to find a branch that covers new nodes.
        auto branch = popPotentialBranch(getVisitedNodes(), *successors);
        // If we succeed, pick the branch and add the remainder to the list of
        // potential branches.
        if (branch.has_value()) {
            auto &nextState = branch.value().nextState;
            potentialBranches.insert(potentialBranches.end(), successors->begin(),
                                     successors->end());
            return nextState;
        }
    }
    // If we can not cover anything new, pick a branch at random.
    auto nextState = popRandomBranch(*successors).nextState;
    // Add the remaining tests to the unexplored branches.
    unexploredBranches.insert(unexploredBranches.end(), successors->begin(), successors->end());
    return nextState;
}

void GreedyNodeSelection::runImpl(const Callback &callBack,
                                  ExecutionStateReference executionState) {
    while (true) {
        try {
            if (executionState.get().isTerminal()) {
                // We've reached the end of the program. Call back and (if desired) end execution.
                bool terminate = handleTerminalState(callBack, executionState);
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
                auto nextState = pickSuccessor(successors);
                if (nextState.has_value()) {
                    executionState = nextState.value();
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
        auto branch = popPotentialBranch(getVisitedNodes(), potentialBranches);
        if (branch.has_value()) {
            executionState = branch.value().nextState;
            continue;
        }
        // We did not find a single branch that could cover new state.
        // Add all potential branches to the list of unexplored branches.
        unexploredBranches.insert(unexploredBranches.end(), potentialBranches.begin(),
                                  potentialBranches.end());
        potentialBranches.clear();
        // If we did not find any new nodes, fall back to random.
        executionState = popRandomBranch(unexploredBranches).nextState;
    }
}

}  // namespace P4Tools::P4Testgen
