#include "backends/p4tools/testgen/core/exploration_strategy/incremental_max_coverage_stack.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/variant/get.hpp>

#include "backends/p4tools/common/lib/coverage.h"
#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/log.h"

#include "backends/p4tools/testgen/core/small_step/small_step.h"
#include "backends/p4tools/testgen/lib/continuation.h"
#include "backends/p4tools/testgen/lib/exceptions.h"
#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

void IncrementalMaxCoverageStack::run(const Callback& callback) {
    // Loop until we reach terminate, or until there are no more
    // branches to produce tests.
    while (true) {
        try {
            if (executionState->isTerminal()) {
                // We've reached the end of the program. Call back and (if desired) end execution.
                bool terminate = handleTerminalState(callback, *executionState);
                if (terminate) {
                    return;
                }
            } else {
                // Take a step in the program, choose a random branch, and continue execution. If
                // branch selection fails, fall through to the roll-back code below. To help reduce
                // calls into the solver, only guarantee viability of the selected branch if more
                // than one branch was produced.
                // State successors are accompanied by branch constrain which should be evaluated
                // in the state before the step was taken - we copy the current symbolic state.
                StepResult successors = step(*executionState);
                bool guaranteeViability = successors->size() > 1;
                ExecutionState* next = chooseBranch(*successors, guaranteeViability);
                if (next != nullptr) {
                    executionState = next;
                    continue;
                }
            }
        } catch (TestgenUnimplemented& e) {
            // If permissive is not enable, we just throw the exception.
            if (!TestgenOptions::get().permissive) {
                throw;
            }
            // Otherwise we try to roll back as we typically do.
            ::warning("Path encountered unimplemented feature. Message: %1%\n", e.what());
        }

        // Roll back to a previous branch and continue execution from there, but if there are no
        // more branches to explore, finish execution. Not all branches are viable, so we loop
        // until either we run out of unexplored branches or we find a viable branch.
        while (true) {
            if (unexploredBranches.empty()) {
                return;
            }
            bool guaranteeViability = true;
            auto successors = unexploredBranches.rbegin()->second;
            // Remove the map entry accordingly.
            auto coverageKey = unexploredBranches.rbegin()->first;
            unexploredBranches.erase(coverageKey);
            ExecutionState* next = chooseBranch(successors, guaranteeViability);
            if (next != nullptr) {
                executionState = next;
                break;
            }
        }
    }
}

IncrementalMaxCoverageStack::IncrementalMaxCoverageStack(AbstractSolver& solver,
                                                         const ProgramInfo& programInfo,
                                                         boost::optional<uint32_t> seed)
    : ExplorationStrategy(solver, programInfo, seed) {}

void IncrementalMaxCoverageStack::sortBranchesByCoverage(std::vector<Branch>& branches) {
    // Transfers branches to rankedBranches and sorts them by coverage
    for (uint64_t i = 0; i < branches.size(); i++) {
        auto localBranch = branches.at(i);
        ExecutionState* branchState = localBranch.nextState;
        // Calculate coverage for each branch:
        uint64_t coverage = 0;
        if (branchState) {
            uint64_t lookAheadCoverage = 0;
            for (const auto& stmt : branchState->getVisited()) {
                // We need to take into account the set of visitedStatements.
                // We also need to ensure the statement is in allStatements.
                if (visitedStatements.count(stmt) == 0U && allStatements.count(stmt) != 0U) {
                    lookAheadCoverage++;
                }
            }
            coverage = lookAheadCoverage + visitedStatements.size();
        } else {
            // If the branch does not have a nextState, then don't look ahead
            // and use other existing information for ranking.
            coverage = visitedStatements.size();
        }

        // If there's no element in unexploredBranches with the particular coverage
        // we calculate, we'll insert a new key at unexploredBranches
        auto rankedBranches = unexploredBranches.find(coverage);
        if (rankedBranches != unexploredBranches.end()) {
            auto& localBranches = rankedBranches->second;
            localBranches.push_back(localBranch);
            unexploredBranches.emplace(coverage, localBranches);
        } else {
            std::vector<Branch> localBranches;
            localBranches.push_back(localBranch);
            unexploredBranches.insert(std::make_pair(coverage, localBranches));
        }
    }

    // Clear branches and reinsert information from rankedBranches
    branches.clear();
}

ExecutionState* IncrementalMaxCoverageStack::chooseBranch(std::vector<Branch>& branches,
                                                          bool guaranteeViability) {
    uint64_t localCoverage = 0;
    // If branches is not empty, we will arrange them by coverage.
    if (!branches.empty()) {
        sortBranchesByCoverage(branches);
        branches = unexploredBranches.rbegin()->second;
        localCoverage = unexploredBranches.rbegin()->first;
    } else {
        return nullptr;
    }

    while (true) {
        // Fail if we've run out of ranked branches.
        if (branches.empty()) {
            // If there are no remaining branches, remove entry from the map
            // before returning.
            unexploredBranches.erase(localCoverage);
            return nullptr;
        }

        // Pick and remove a branch randomly. All branches in this vector will have the same
        // coverage.
        auto idx = selectBranch(branches);
        auto branch = branches.at(idx);
        // Note: This could be improved, because we should remove only succeeded selection.
        // Unsatisfiable branches could be covered in other tests after backtracking.
        branches.erase(branches.begin() + idx);

        // Do not bother invoking the solver for a trivial case.
        // In either case (true or false), we do not need to add the assertion and check.
        if (const auto* boolLiteral = branch.constraint->to<IR::BoolLiteral>()) {
            guaranteeViability = false;
            if (!boolLiteral->value) {
                continue;
            }
        }

        if (guaranteeViability) {
            // Check the consistency of the path constraints asserted so far.
            auto solverResult = solver.checkSat(branch.nextState->getPathConstraint());
            if (solverResult == boost::none) {
                ::warning("Solver timed out");
            }
            if (solverResult == boost::none || !solverResult.get()) {
                // Solver timed out or path constraints were not satisfiable. Need to choose a
                // different branch. Roll back our branch selection and try again.
                continue;
            }
        }
        // Push the new set of branches if the remaining vector is not empty.
        if (branches.size() > 0) {
            auto coverageKey = unexploredBranches.find(localCoverage);
            coverageKey->second = branches;
        } else {
            // If there are no remaining branches, remove entry from the map
            // before returning.
            unexploredBranches.erase(localCoverage);
        }
        return branch.nextState;
    }
}

}  // namespace P4Testgen

}  // namespace P4Tools
