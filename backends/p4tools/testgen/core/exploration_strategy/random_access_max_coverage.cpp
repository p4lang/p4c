#include "backends/p4tools/testgen/core/exploration_strategy/random_access_max_coverage.h"

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
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/irutils.h"
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

void RandomAccessMaxCoverage::run(const Callback& callback) {
    // Loop until we reach terminate, or until there are no more
    // branches to produce tests.
    while (true) {
        try {
            if (executionState->isTerminal()) {
                // We've reached the end of the program. Call back and (if desired) end execution.
                bool terminate = handleTerminalState(callback, *executionState);
                uint64_t coverage = visitedStatements.size();
                // We set the coverage saddle track accordingly.
                if (coverage == coverageSaddleTrack.first) {
                    coverageSaddleTrack =
                        std::make_pair(coverage, (coverageSaddleTrack.second + 1));
                } else {
                    coverageSaddleTrack = std::make_pair(coverage, 1);
                }
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
        while (true) {
            // If the buffer and unexploredBranches itself are empty, there's nothing left to
            // explore.
            if (unexploredBranches.empty() && bufferUnexploredBranches.empty()) {
                return;
            }

            bool guaranteeViability = true;
            ExecutionState* next = nullptr;
            if (unexploredBranches.empty() && !bufferUnexploredBranches.empty()) {
                // If we don't have enough entires, then we just invoke the
                // regular maxCoverage logic, i.e. we select the branches with
                // higher number of unique non-visited statements.
                auto successors = bufferUnexploredBranches.rbegin()->second;
                // Remove the map entry accordingly.
                auto coverageKey = bufferUnexploredBranches.rbegin()->first;
                bufferUnexploredBranches.erase(coverageKey);
                next = chooseBranch(successors, guaranteeViability);
            } else {
                // If we are stuck in a sadlle point for too long,
                // trust in the lookahead and take the higher ranks.
                if (coverageSaddleTrack.second >= (2 * saddlePoint)) {
                    // Empty unexploredBranches.
                    updateBufferRankings();
                    // Merge the contents of unexploredBranches in the buffer, then
                    // empty it.
                    for (auto& localBranches : unexploredBranches) {
                        sortBranchesByCoverage(localBranches);
                    }
                    unexploredBranches.clear();
                    // We set the coverage counter to the saddle point,
                    // so we can combine it with random exploration.
                    uint64_t coverage = visitedStatements.size();
                    coverageSaddleTrack = std::make_pair(coverage, saddlePoint);
                    // Get the highest rank in the map in terms of non-
                    // visited statements.
                    auto successors = bufferUnexploredBranches.rbegin()->second;
                    // Remove the map entry accordingly.
                    auto coverageKey = bufferUnexploredBranches.rbegin()->first;
                    bufferUnexploredBranches.erase(coverageKey);
                    next = chooseBranch(successors, guaranteeViability);
                } else if (coverageSaddleTrack.second >= saddlePoint) {
                    // Invoke random access iff we reach a saddle point.
                    updateBufferRankings();
                    // Merge the contents of unexploredBranches in the buffer, then
                    // empty it.
                    for (auto& localBranches : unexploredBranches) {
                        sortBranchesByCoverage(localBranches);
                    }
                    unexploredBranches.clear();
                    auto successorsKey = getRandomUnexploredMapEntry();
                    auto successors = bufferUnexploredBranches.at(successorsKey);
                    // Remove the map entry accordingly.
                    bufferUnexploredBranches.erase(successorsKey);
                    next = chooseBranch(successors, guaranteeViability);
                } else {
                    // Otherwise, just get the first element.
                    auto successors = unexploredBranches.front();
                    unexploredBranches.pop_front();
                    next = chooseBranch(successors, guaranteeViability);
                }
            }
            if (next != nullptr) {
                executionState = next;
                break;
            }
        }
    }
}

uint64_t RandomAccessMaxCoverage::getRandomUnexploredMapEntry() {
    // Collect all the keys and select a random one.
    std::vector<uint64_t> unexploredCoverageKeys;
    for (auto const& unexplored : bufferUnexploredBranches) {
        unexploredCoverageKeys.push_back(unexplored.first);
    }
    size_t unexploredRange = unexploredCoverageKeys.size() - 1;
    uint64_t exploreLevels = Utils::getRandInt(unexploredRange);
    uint64_t coverageKey = unexploredCoverageKeys.at(exploreLevels);

    return coverageKey;
}

void RandomAccessMaxCoverage::updateBufferRankings() {
    // Collect all the keys
    std::vector<uint64_t> unexploredCoverageKeys;
    for (auto const& unexplored : bufferUnexploredBranches) {
        unexploredCoverageKeys.push_back(unexplored.first);
    }
    for (auto& coverageKey : unexploredCoverageKeys) {
        auto localBranches = bufferUnexploredBranches.at(coverageKey);
        bufferUnexploredBranches.erase(coverageKey);
        sortBranchesByCoverage(localBranches);
    }
}

void RandomAccessMaxCoverage::sortBranchesByCoverage(std::vector<Branch>& branches) {
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

        // If there's no element in bufferUnexploredBranches with the particular coverage
        // we calculate, we'll insert a new key at bufferUnexploredBranches.
        auto rankedBranches = bufferUnexploredBranches.find(coverage);
        if (rankedBranches != bufferUnexploredBranches.end()) {
            auto& localBranches = rankedBranches->second;
            localBranches.push_back(localBranch);
            bufferUnexploredBranches.emplace(coverage, localBranches);
        } else {
            std::vector<Branch> localBranches;
            localBranches.push_back(localBranch);
            bufferUnexploredBranches.insert(std::make_pair(coverage, localBranches));
        }
    }

    // Clear branches and reinsert information from rankedBranches
    branches.clear();
}

ExecutionState* RandomAccessMaxCoverage::chooseBranch(std::vector<Branch>& branches,
                                                      bool guaranteeViability) {
    while (true) {
        // Fail if we've run out of ranked branches.
        if (branches.empty()) {
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
            unexploredBranches.push_back(branches);
        }

        return branch.nextState;
    }
}

RandomAccessMaxCoverage::RandomAccessMaxCoverage(AbstractSolver& solver,
                                                 const ProgramInfo& programInfo,
                                                 boost::optional<uint32_t> seed,
                                                 uint64_t saddlePoint)
    : IncrementalMaxCoverageStack(solver, programInfo, seed), saddlePoint(saddlePoint) {}

}  // namespace P4Testgen

}  // namespace P4Tools
