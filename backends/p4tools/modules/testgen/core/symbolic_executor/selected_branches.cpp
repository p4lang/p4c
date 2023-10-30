#include "backends/p4tools/modules/testgen/core/symbolic_executor/selected_branches.h"

#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "ir/solver.h"
#include "lib/error.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

void SelectedBranches::runImpl(const Callback &callBack, ExecutionStateReference executionState) {
    try {
        while (!executionState.get().isTerminal()) {
            StepResult successors = step(executionState);
            // Assign branch ids to the branches. These integer branch ids are used by
            // track-branches and selected (input) branches features. Also populates
            // exploredBranches from the initial set of branches.
            for (uint64_t bIdx = 0; bIdx < successors->size(); ++bIdx) {
                auto &succ = (*successors)[bIdx];
                succ.nextState.get().pushBranchDecision(bIdx + 1);
            }
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
            auto *next = chooseBranch(*successors, nextBranch);
            if (next == nullptr) {
                break;
            }
            executionState = *next;
        }
        if (executionState.get().isTerminal()) {
            // We've reached the end of the program. Call back and end execution.
            handleTerminalState(callBack, executionState);
            if (!selectedBranches.empty()) {
                ::warning(
                    "Execution reached a final state before executing whole "
                    "selected path!");
            }
            return;
        }
    } catch (...) {
        if (TestgenOptions::get().trackBranches) {
            // Print list of the selected branches and store all information into
            // dumpFolder/selectedBranches.txt file.
            // This printed list could be used for repeat this bug in arguments of --input-branches
            // command line. For example, --input-branches "1,1".
            printCurrentTraceAndBranches(std::cerr, executionState);
        }
        throw;
    }
}

uint64_t getNumeric(const std::string &str) {
    char *leftString = nullptr;
    uint64_t number = strtoul(str.c_str(), &leftString, 10);
    BUG_CHECK(!(*leftString), "Can't translate selected branch %1% into int", str);
    return number;
}

SelectedBranches::SelectedBranches(AbstractSolver &solver, const ProgramInfo &programInfo,
                                   std::string selectedBranchesStr)
    : SymbolicExecutor(solver, programInfo) {
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

ExecutionState *SelectedBranches::chooseBranch(const std::vector<Branch> &branches,
                                               uint64_t nextBranch) {
    for (const auto &branch : branches) {
        const auto &selectedBranches = branch.nextState.get().getSelectedBranches();
        BUG_CHECK(!selectedBranches.empty(), "Corrupted selectedBranches in a execution state");
        // Find branch matching given branch identifier.
        if (selectedBranches.back() == nextBranch) {
            return &branch.nextState.get();
        }
    }
    // If not found, the input selected branch list is invalid.
    ::error("The selected branches string doesn't match any branch.");

    return nullptr;
}

}  // namespace P4Tools::P4Testgen
