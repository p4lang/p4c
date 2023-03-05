#include "backends/p4tools/modules/testgen/core/exploration_strategy/greedy_potential.h"

#include <vector>

#include <boost/none.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/util.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/timer.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

ExecutionState *evaluateBranch(const ExplorationStrategy::Branch &branch, bool guaranteeViability,
                               AbstractSolver &solver) {
    // Do not bother invoking the solver for a trivial case.
    // In either case (true or false), we do not need to add the assertion and check.
    if (const auto *boolLiteral = branch.constraint->to<IR::BoolLiteral>()) {
        guaranteeViability = false;
        if (!boolLiteral->value) {
            return nullptr;
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
            return nullptr;
        }
    }

    // Branch selection succeeded.
    return branch.nextState;
}

ExplorationStrategy::Branch popBranch(const P4::Coverage::CoverageSet &coveredStatements,
                                      std::vector<ExplorationStrategy::Branch> &candidateBranches) {
    for (size_t idx = 0; idx < candidateBranches.size(); ++idx) {
        auto branch = candidateBranches.at(idx);
        // First check all the potential set of statements we can cover by looking ahead.
        for (const auto &stmt : branch.potentialStatements) {
            // We need to take into account the set of visitedStatements.
            if (coveredStatements.count(stmt) == 0U) {
                candidateBranches[idx] = candidateBranches.back();
                candidateBranches.pop_back();
                return branch;
            }
        }
        // If we did not find anything, check whether this state covers any new statements
        // already.
        for (const auto &stmt : branch.nextState->getVisited()) {
            if (coveredStatements.count(stmt) == 0U) {
                candidateBranches[idx] = candidateBranches.back();
                candidateBranches.pop_back();
                return branch;
            }
        }
    }

    // If we did not find any new statements, fall back to random.
    auto branchIdx = Utils::getRandInt(candidateBranches.size() - 1);
    auto branch = candidateBranches[branchIdx];
    candidateBranches[branchIdx] = candidateBranches.back();
    candidateBranches.pop_back();
    return branch;
}

GreedyPotential::GreedyPotential(AbstractSolver &solver, const ProgramInfo &programInfo)
    : ExplorationStrategy(solver, programInfo) {}

void GreedyPotential::run(const Callback &callback) {
    while (true) {
        try {
            if (executionState->isTerminal()) {
                // We've reached the end of the program. Call back and (if desired) end execution.
                bool terminate = handleTerminalState(callback, *executionState);
                stepsWithoutTest = 0;
                if (terminate) {
                    return;
                }
            } else {
                // Take a step in the program, choose a random branch, and continue execution. If
                // branch selection fails, fall through to the roll-back code below. To help reduce
                // calls into the solver, only guarantee viability of the selected branch if more
                // than one branch was produced.
                // State successors are accompanied by branch constraint which should be evaluated
                // in the state before the step was taken - we copy the current symbolic state.
                StepResult successors = step(*executionState);

                // If there is only one successor, choose it and move on.
                if (successors->size() == 1) {
                    executionState = successors->at(0).nextState;
                    continue;
                }

                // If there are multiple successors, try to pick one.
                if (successors->size() > 1) {
                    //. In case the strategy gets stuck because of its greedy behavior, fall back to
                    // randomness after MAX_STEPS_WITHOUT_TEST branch decisions without a result.
                    stepsWithoutTest++;
                    auto branch = popBranch(getVisitedStatements(), *successors);
                    unexploredBranches.insert(unexploredBranches.end(), successors->begin(),
                                              successors->end());

                    auto *next = evaluateBranch(branch, true, solver);
                    if (next != nullptr) {
                        executionState = next;
                        continue;
                    }
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
        while (true) {
            if (unexploredBranches.empty()) {
                return;
            }
            Util::ScopedTimer chooseBranchtimer("branch_selection");
            auto branch = popBranch(getVisitedStatements(), unexploredBranches);
            auto *next = evaluateBranch(branch, true, solver);
            if (next != nullptr) {
                executionState = next;
                break;
            }
        }
    }
}

}  // namespace P4Tools::P4Testgen
