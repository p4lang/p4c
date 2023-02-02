#include "backends/p4tools/modules/testgen/core/exploration_strategy/unbounded_random_access_stack.h"

#include <ctime>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/util.h"
#include "lib/error.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/incremental_stack.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

void UnboundedRandomAccessStack::run(const Callback &callback) {
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
                ExecutionState *next = chooseBranch(*successors, guaranteeViability);
                if (next != nullptr) {
                    executionState = next;
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
        while (true) {
            if (unexploredBranches.empty() && bufferUnexploredBranches.empty()) {
                return;
            }
            bool guaranteeViability = true;
            ExecutionState *next = nullptr;
            // Here we restore unexploredBranches from the local buffer
            if (unexploredBranches.empty() && !bufferUnexploredBranches.empty()) {
                for (size_t i = 0; i < bufferUnexploredBranches.size(); i++) {
                    auto *branches = bufferUnexploredBranches.front();
                    bufferUnexploredBranches.pop_front();
                    unexploredBranches.push(branches);
                }
                auto *successors = unexploredBranches.pop();
                next = chooseBranch(*successors, guaranteeViability);
            } else {
                auto *successors = multiPop(unexploredBranches);
                next = chooseBranch(*successors, guaranteeViability);
            }
            if (next != nullptr) {
                executionState = next;
                break;
            }
        }
    }
}

UnboundedRandomAccessStack::UnboundedRandomAccessStack(AbstractSolver &solver,
                                                       const ProgramInfo &programInfo)
    : IncrementalStack(solver, programInfo) {}

UnboundedRandomAccessStack::StepResult UnboundedRandomAccessStack::multiPop(
    IncrementalStack::UnexploredBranches &unexploredBranches) {
    // Consider the entire stack size as a possible range.
    auto unexploredRange = unexploredBranches.size() - 1;
    auto popLevels = Utils::getRandInt(unexploredRange) + 1;
    // Saves unexploredBranches in the buffer if we are popping more
    // than one level to revist them later.
    if (popLevels == 1) {
        return unexploredBranches.pop();
    }
    for (uint64_t index = 0; index < (popLevels - 1); index++) {
        auto *unexploredSuccessors = unexploredBranches.pop();
        if (!(unexploredSuccessors->empty())) {
            bufferUnexploredBranches.push_back(unexploredSuccessors);
        }
    }
    return unexploredBranches.pop();
}

}  // namespace P4Testgen

}  // namespace P4Tools
