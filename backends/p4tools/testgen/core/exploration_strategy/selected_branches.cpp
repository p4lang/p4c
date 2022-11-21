#include "backends/p4tools/testgen/core/exploration_strategy/selected_branches.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gsl/gsl-lite.hpp"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "p4tools/common/core/solver.h"

#include "p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "p4tools/testgen/core/program_info.h"
#include "p4tools/testgen/core/small_step/small_step.h"

namespace P4Tools {

namespace P4Testgen {

void SelectedBranches::run(const Callback& callback) {
    while (!executionState->isTerminal()) {
        StepResult successors = step(*executionState);
        if (successors->size() == 1) {
            // Non-branching states are not recorded by selected branches.
            executionState = (*successors)[0].nextState;
            continue;
        }
        if (selectedBranches.empty()) {
            // Not enough steps in the input selected branches string, cannot continue.
            ::warning("The selected path is incomplete, not emitting a testcase.");
            break;
        }
        // If there are multiple, pop one branch decision from the input list and pick
        // successor matching the given branch decision.
        uint64_t nextBranch = selectedBranches.front();
        selectedBranches.pop_front();
        ExecutionState* next = chooseBranch(*successors, nextBranch);
        if (next == nullptr) {
            break;
        }
        executionState = next;
    }
    if (executionState->isTerminal()) {
        // We've reached the end of the program. Call back and (if desired) end execution.
        handleTerminalState(callback, *executionState);
        if (!selectedBranches.empty()) {
            ::warning(
                "Execution reached a final state before executing whole "
                "selected path!");
        }
        return;
    }
}

uint64_t getNumeric(const std::string& str) {
    char* leftString = nullptr;
    uint64_t number = strtoul(str.c_str(), &leftString, 10);
    BUG_CHECK(!(*leftString), "Can't translate selected branch %1% into int", str);
    return number;
}

SelectedBranches::SelectedBranches(AbstractSolver& solver, const ProgramInfo& programInfo,
                                   boost::optional<uint32_t> seed, std::string selectedBranchesStr)
    : ExplorationStrategy(solver, programInfo, seed) {
    size_t n = 0;
    auto str = std::move(selectedBranchesStr);
    while ((n = str.find(',')) != std::string::npos) {
        selectedBranches.push_back(getNumeric(str.substr(0, n)));
        str = str.substr(n + 1);
    }
    if (str.length() != 0U) {
        selectedBranches.push_back(getNumeric(str));
    }
}

ExecutionState* SelectedBranches::chooseBranch(const std::vector<Branch>& branches,
                                               uint64_t nextBranch) {
    ExecutionState* next = nullptr;
    for (const auto& branch : branches) {
        const auto& selectedBranches = branch.nextState->getSelectedBranches();
        BUG_CHECK(!selectedBranches.empty(), "Corrupted selectedBranches in a execution state");
        // Find branch matching given branch identifier.
        if (selectedBranches.back() == nextBranch) {
            next = branch.nextState;
            break;
        }
    }

    if (!next) {
        // If not found, the input selected branch list is invalid.
        ::error("The selected branches string doesn't match any branch.");
    }

    return next;
}

}  // namespace P4Testgen

}  // namespace P4Tools
