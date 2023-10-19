#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_SELECTED_BRANCHES_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_SELECTED_BRANCHES_H_

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// Explores one path described by a list of branches.
class SelectedBranches : public SymbolicExecutor {
 public:
    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    void runImpl(const Callback &callBack, ExecutionStateReference executionState) override;

    /// Constructor for this strategy, considering inheritance
    SelectedBranches(AbstractSolver &solver, const ProgramInfo &programInfo,
                     std::string selectedBranchesStr);

 private:
    /// Chooses a branch corresponding to a given branch identifier.
    ///
    /// @returns next execution state to be examined, throws an exception on invalid nextBranch.
    ExecutionState *chooseBranch(const std::vector<Branch> &branches, uint64_t nextBranch);

    /// The list of selected branches.
    std::list<uint64_t> selectedBranches;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_SELECTED_BRANCHES_H_ */
