#ifndef TESTGEN_CORE_EXPLORATION_STRATEGY_INCREMENTAL_MAX_COVERAGE_STACK_H_
#define TESTGEN_CORE_EXPLORATION_STRATEGY_INCREMENTAL_MAX_COVERAGE_STACK_H_

#include <cstdint>
#include <ctime>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"

#include "testgen/common/core/solver.h"
#include "testgen/common/lib/formulae.h"
#include "testgen/testgen/core/exploration_strategy/exploration_strategy.h"
#include "testgen/testgen/core/program_info.h"
#include "testgen/testgen/core/small_step/small_step.h"
#include "testgen/testgen/lib/execution_state.h"
#include "testgen/testgen/lib/final_state.h"

namespace P4Tools {

namespace P4Testgen {

/// Strategy that tries to maximize coverage by getting new branches
/// and sorting them by uncovered states. We "look-ahead" at the possible next execution
/// states for each branch and identify the number of non-visited states. We rank the
/// branches so those with more states to be explored are picked first.
class IncrementalMaxCoverageStack : public ExplorationStrategy {
 public:
    /// Callbacks are invoked when the P4 program terminates. If the callback returns true,
    /// execution halts. Otherwise, execution of the P4 program continues on a different random
    /// path.
    using Callback = std::function<bool(const FinalState&)>;

    using Branch = ExplorationStrategy::Branch;
    using StepResult = ExplorationStrategy::StepResult;

    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    void run(const Callback& callBack) override;

    // void setAllStatementsSize(int allStatementsSize);

    /// Constructor for this strategy, considering inheritance.
    IncrementalMaxCoverageStack(AbstractSolver& solver, const ProgramInfo& programInfo,
                                boost::optional<uint32_t> seed);

 protected:
    /// The main datastructure for exploration in this strategy. It is a map
    /// of pairs, and each pair carries the potential coverage and a vector of
    /// branches with that same coverage.
    std::map<uint64_t, std::vector<Branch>> unexploredBranches;

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
    ExecutionState* chooseBranch(std::vector<Branch>& branches, bool guaranteeViability);

    /// Invoked in chooseBranch to sort the branches vector according to non-visited
    /// states.
    void sortBranchesByCoverage(std::vector<Branch>& branches);
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_CORE_EXPLORATION_STRATEGY_INCREMENTAL_MAX_COVERAGE_STACK_H_ */
