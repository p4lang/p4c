#include "backends/p4tools/testgen/core/exploration_strategy/sourceTest_generation.h"

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

void SourceTest::run(const Callback& callback) {
    ExecutionState* executionState = new ExecutionState(programInfo.program);
    SmallStepEvaluator evaluator(solver, programInfo);
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
                StepResult successors = evaluator.step(*executionState);
                bool guaranteeViability = successors->size() > 1;
                ExecutionState* next = chooseBranch(*successors, guaranteeViability);
                if (next) {
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
            auto successors = unexploredBranches.pop();
            ExecutionState* next = chooseBranch(*successors, guaranteeViability);
            if (next) {
                executionState = next;
                break;
            }
        }
    }
}

SourceTest::SourceTest(AbstractSolver& solver, const ProgramInfo& programInfo,
                       boost::optional<uint32_t> seed)
    : IncrementalStack(solver, programInfo, seed) {}

ExecutionState* SourceTest::chooseBranch(std::vector<Branch>& branches, bool guaranteeViability) {
    // Store index of a branch for first branch traversal.
    // initialBranches contains all elements with corresponded index in
    // the branches data structure before erasing some elements from it.
    if (branches.size() > 1) {
        for (size_t i = 0; i < branches.size(); i++) {
            // If an element was found then it isn't new and we don't add it.
            // In that case some of the elements were removed from branches
            // data structure previously and indexes weren't the same as for
            // the first branch traversal.
            if (initialBranches.count(branches.at(i).nextState) != 0U) {
                continue;
            }
            initialBranches.emplace(branches.at(i).nextState, i);
        }
    }
    while (true) {
        // Fail if we've run out of branches.
        if (branches.empty()) {
            return nullptr;
        }
        // Pick and remove a branch.
        auto idx = selectBranch(branches);
        auto branch = branches.at(idx);

        // Note: This could be improved, because we should remove only succeeded selection.
        // Unsatisfiable branches could be covered in other tests after backtracking.
        branches.erase(branches.begin() + idx);

        // Save the current solver state and push the new set of unexplored branches.
        unexploredBranches.push(&branches);

        // Do not bother invoking the solver for a trivial case.
        // In either case (true or false), we do not need to add the assertion and check.
        if (const auto* boolLiteral = branch.constraint->to<IR::BoolLiteral>()) {
            guaranteeViability = false;
            if (!boolLiteral->value) {
                unexploredBranches.pop();
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
                unexploredBranches.pop();
                continue;
            }
        }

        if (!branch.nextState->getBody().empty()) {
            const auto cmd = branch.nextState->getBody().next();
            if (const auto* nd = boost::get<const IR::Node*>(&cmd)) {
                branch.nextState->add(new TraceEvent::SourceInfo(*nd, idx));
            }
        }
        // Branch selection succeeded.
        return branch.nextState;
    }
}

}  // namespace P4Testgen

}  // namespace P4Tools