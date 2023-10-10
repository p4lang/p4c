#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_SYMBOLIC_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_SYMBOLIC_EXECUTOR_H_

#include <functional>
#include <iosfwd>
#include <vector>

#include "ir/solver.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/final_state.h"

namespace P4Tools::P4Testgen {

/// Base abstract class for symbolic execution. It requires the implementation of
/// the run method, and carries the base Branch struct, to be reused in inherited
/// classes. It also holds a default termination method, which can be overridden.
class SymbolicExecutor {
 public:
    virtual ~SymbolicExecutor() = default;

    SymbolicExecutor(const SymbolicExecutor &) = default;

    SymbolicExecutor(SymbolicExecutor &&) = delete;

    SymbolicExecutor &operator=(const SymbolicExecutor &) = delete;

    SymbolicExecutor &operator=(SymbolicExecutor &&) = delete;

    /// Callbacks are invoked when the P4 program terminates. If the callback returns true,
    /// execution halts. Otherwise, execution of the P4 program continues on a different random
    /// path.
    using Callback = std::function<bool(const FinalState &)>;

    using Branch = SmallStepEvaluator::Branch;

    using StepResult = SmallStepEvaluator::Result;

    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    virtual void run(const Callback &callBack);

    virtual void runImpl(const Callback &callBack, ExecutionStateReference executionState) = 0;

    explicit SymbolicExecutor(AbstractSolver &solver, const ProgramInfo &programInfo);

    /// Writes a list of the selected branches into @param out.
    void printCurrentTraceAndBranches(std::ostream &out, const ExecutionState &executionState);

    /// Getter to access visitedNodes.
    const P4::Coverage::CoverageSet &getVisitedNodes();

    /// Update the set of visited nodes. Returns true if there was an update.
    [[nodiscard]] bool updateVisitedNodes(const P4::Coverage::CoverageSet &newNodes);

 protected:
    /// Target-specific information about the P4 program.
    const ProgramInfo &programInfo;

    /// The SMT solver backing this executor.
    AbstractSolver &solver;

    /// Set of all nodes, to be retrieved from programInfo.
    const P4::Coverage::CoverageSet &coverableNodes;

    /// Set of all nodes executed in any testcase that has been outputted.
    P4::Coverage::CoverageSet visitedNodes;

    /// Handles processing at the end of a P4 program.
    ///
    /// @returns true if symbolic execution should end; false if symbolic execution should continue
    /// on a different path.
    bool handleTerminalState(const Callback &callback, const ExecutionState &terminalState);

    /// Take one step in the program and return list of possible branches.
    StepResult step(ExecutionState &state);

    /// Take a branch and a solver as input.
    /// Compute the branch's path conditions using the solver.
    /// Return true if the solver can find a solution and does not time out.
    static bool evaluateBranch(const SymbolicExecutor::Branch &branch, AbstractSolver &solver);

    /// Select a branch at random from the input @param candidateBranches.
    //  Remove the branch from the container.
    static SymbolicExecutor::Branch popRandomBranch(
        std::vector<SymbolicExecutor::Branch> &candidateBranches);

 private:
    SmallStepEvaluator evaluator;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_SYMBOLIC_EXECUTOR_H_ */
