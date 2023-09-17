#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_RANDOM_BACKTRACK_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_RANDOM_BACKTRACK_H_

#include <vector>

#include "lib/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"

namespace P4Tools::P4Testgen {

/// A simple random traversal strategy.
/// If this selection strategy hits a branch point, it will choose a path at random and continue
/// execution downwards. Once the selection strategy has hit a terminal state, it will pop a random
/// state from the container of unexplored, remaining branches.
class RandomBacktrack : public SymbolicExecutor {
 public:
    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    void runImpl(const Callback &callBack, ExecutionStateReference executionState) override;

    /// Constructor for this strategy, considering inheritance
    RandomBacktrack(AbstractSolver &solver, const ProgramInfo &programInfo);

 private:
    /// General unexplored branches.
    // Each element on this vector represents a set of alternative choices that could have been
    /// made along the current execution path.
    ///
    /// Invariants:
    ///   - Each element of this vector is non-empty.
    ///   - Each element's path constraints are satisfiable.
    ///   - There are no nodes associated with the element's execution state that are
    ///   uncovered.
    std::vector<Branch> unexploredBranches;

    /// Try to pick a successor from the list of given successors. This involves three steps.
    /// 1. Filter out all the successors with unsatisfiable path conditions. If no successors are
    /// left, return false.
    /// 2. If there are successors left, try to find a successor that covers new nodes. Set the
    /// nextState as this successors state.
    /// 3. If no successor with new nodes was found set a random successor.
    [[nodiscard]] std::optional<ExecutionStateReference> pickSuccessor(StepResult successors);
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_RANDOM_BACKTRACK_H_ */
