#ifndef BACKENDS_P4TOOLS_TESTGEN_CORE_EXPLORATION_STRATEGY_SOURCETEST_GENERATION_H_
#define BACKENDS_P4TOOLS_TESTGEN_CORE_EXPLORATION_STRATEGY_SOURCETEST_GENERATION_H_

#include <cstdint>
#include <ctime>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"

#include "backends/p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/testgen/core/exploration_strategy/incremental_stack.h"
#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/lib/final_state.h"

namespace P4Tools {

namespace P4Testgen {

/// For now, a stack-based approach that track branches when --track-branches
/// is activated. TODO: to be refactored and transformed into a decorator, so it
/// can be used with other strategies and not only stacks.
class SourceTest : public IncrementalStack {
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

    /// Constructor for this strategy, considering inheritance
    SourceTest(AbstractSolver& solver, const ProgramInfo& programInfo,
               boost::optional<uint32_t> seed);

 private:
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

    /// The output for the passed branches.
    std::ostringstream branchesOutput;

    /// An execution state to number of a branch in first test.
    std::map<const ExecutionState*, uint64_t> initialBranches;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_CORE_EXPLORATION_STRATEGY_SOURCETEST_GENERATION_H_ */
