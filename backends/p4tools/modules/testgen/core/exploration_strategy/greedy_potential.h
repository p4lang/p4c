#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_GREEDY_POTENTIAL_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_GREEDY_POTENTIAL_H_

#include <cstdint>
#include <ctime>
#include <stack>
#include <vector>

#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

/// Simple incremental stack strategy. It explores paths and stores them in a stack
/// encapsulated in UnexploredBranches inner class. This strategy aims to match
/// the solver incremental feature, which also uses a stack. We push and pop single
/// elements accordingly.
class GreedyPotential : public ExplorationStrategy {
 public:
    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    void run(const Callback &callBack) override;

    /// Constructor for this strategy, considering inheritance
    GreedyPotential(AbstractSolver &solver, const ProgramInfo &programInfo);

 protected:
    /// This variable keeps track of how many branch decisions we have made without producing a
    /// test. This is a safety guard in case the strategy gets stuck in parser loops because of its
    /// greediness.
    uint64_t stepsWithoutTest = 0;
    /// Encapsulates a stack of unexplored branches. This exists to help enforce the invariant that
    /// any push or pop operation on this stack should be paired with a corresponding push/pop
    /// operation on the solver.
    class UnexploredBranches {
     public:
        [[nodiscard]] bool empty() const;
        void push(StepResult branches);
        Branch pop(const P4::Coverage::CoverageSet &coveredStatements, bool search,
                   bool fallBackToRandom);
        size_t size();

        UnexploredBranches();

     private:
        /// The threshold increments every time we are in search modes, but can not find an
        /// execution state with uncovered statements.
        uint64_t threshold = 0;
        /// Each element on this stack represents a set of alternative choices that could have been
        /// made along the current execution path.
        ///
        /// Invariants:
        ///   - Each element of this stack is non-empty.
        ///   - Each time we push or pop this stack, we also push or pop on the SMT solver.
        std::vector<StepResult> unexploredBranches;

        int findBranch(const P4::Coverage::CoverageSet &coveredStatements, bool search);
    };

    /// A stack, wherein each element represents a set of alternative choices that could have been
    /// made along the current execution path.
    UnexploredBranches unexploredBranches;

    /// Chooses a branch to take, sets the current execution state to be that branch, and asserts
    /// the corresponding path constraint to the solver.
    ///
    /// If @arg guaranteeViability is true, then the cumulative path condition is guaranteed to be
    /// satisfiable after taking into account the path constraint associated with the chosen
    /// branch. Any unviable branches encountered during branch selection are discarded.
    ///
    /// Branch selection fails if the given set of branches is empty. It also fails if @arg
    /// guaranteeViability is true, but none of the given branches are viable.
    ///
    /// On success, the remaining branches to explore are pushed onto the stack of unexplored
    /// branches, and the solver's state before the branch selection is saved with a `push`
    /// operation. This happens even if the set of remaining branches is empty. On failure, the
    /// stack of unexplored branches and the solver's state will be unchanged.
    ///
    /// @returns next execution state to be examined on success, nullptr on failure.
    ExecutionState *chooseBranch(bool guaranteeViability, bool search,
                                 bool fallBackToRandom = false);
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_GREEDY_POTENTIAL_H_ */
