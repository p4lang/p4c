#include "backends/p4tools/testgen/core/exploration_strategy/incremental_stack.h"

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

#include "backends/p4tools/testgen/lib/continuation.h"
#include "backends/p4tools/testgen/lib/exceptions.h"
#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

void IncrementalStack::run(const Callback& callback) {
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
            auto* successors = unexploredBranches.pop();
            ExecutionState* next = chooseBranch(*successors, guaranteeViability);
            if (next != nullptr) {
                executionState = next;
                break;
            }
        }
    }
}

IncrementalStack::IncrementalStack(AbstractSolver& solver, const ProgramInfo& programInfo,
                                   boost::optional<uint32_t> seed)
    : ExplorationStrategy(solver, programInfo, seed) {}

ExecutionState* IncrementalStack::chooseBranch(std::vector<Branch>& branches,
                                               bool guaranteeViability) {
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

        // Branch selection succeeded.
        return branch.nextState;
    }
}

bool IncrementalStack::UnexploredBranches::empty() const { return unexploredBranches.empty(); }

void IncrementalStack::UnexploredBranches::push(IncrementalStack::StepResult branches) {
    unexploredBranches.push(branches);
}

IncrementalStack::StepResult IncrementalStack::UnexploredBranches::pop() {
    auto* result = unexploredBranches.top();
    unexploredBranches.pop();
    return result;
}

size_t IncrementalStack::UnexploredBranches::size() { return unexploredBranches.size(); }

IncrementalStack::UnexploredBranches::UnexploredBranches() = default;

}  // namespace P4Testgen

}  // namespace P4Tools
