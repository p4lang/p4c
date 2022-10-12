#ifndef BACKENDS_P4TOOLS_TESTGEN_CORE_EXPLORATION_STRATEGY_RANDOM_ACCESS_MAX_COVERAGE_H_
#define BACKENDS_P4TOOLS_TESTGEN_CORE_EXPLORATION_STRATEGY_RANDOM_ACCESS_MAX_COVERAGE_H_

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

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/testgen/core/exploration_strategy/incremental_max_coverage_stack.h"
#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/core/small_step/small_step.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/lib/final_state.h"

namespace P4Tools {

namespace P4Testgen {

/// Strategy that combines the incremental max coverage ("look-ahead") at the
/// with random exploration. We rely on a sorted map containing rankings of unique
/// non-visited statements and a vector, acting as a DFS, to direct a path to a
/// terminating state. We keep track of coverage, and once we reach a saddle point,
/// we pick a random branch. If we continue in the saddle point after for too long
// i.e. 2* the saddle point, we try to stick with the highest ranked branches.
class RandomAccessMaxCoverage : public IncrementalMaxCoverageStack {
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

    /// Constructor for this strategy, considering inheritance.
    RandomAccessMaxCoverage(AbstractSolver& solver, const ProgramInfo& programInfo,
                            boost::optional<uint32_t> seed, uint64_t saddlePoint);

 protected:
    // Saddle point indicates when we get stuck into a coverage and decides to take
    // a random branch. Set by the user.
    uint64_t saddlePoint;

    // Explores other options in the bufferUnexploredBranches map with smaller visitedStatements.
    // It randomly picks a key with a non-max number of unique non-visited statements to explore.
    // Invoked when we reach the saddle point.
    uint64_t getRandomUnexploredMapEntry();

    // A vector buffering the unexplored branches and directing it to a terminal state.
    std::list<std::vector<Branch>> unexploredBranches;

    // First element is the number of non-visited statements, second element is the count
    // of generated tests.
    std::pair<uint64_t, uint64_t> coverageSaddleTrack = std::make_pair(0, 0);

    // Buffer of unexploredBranches. It saves the unexplored branches,
    // so we can restore them if getRandomUnexploredMapEntry finishes a path
    // in unexploredBranches.
    std::map<uint64_t, std::vector<Branch>> bufferUnexploredBranches;

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
    /// states. It stores and ranks the branches from unexploredBranches.
    void sortBranchesByCoverage(std::vector<Branch>& branches);

    // Every time we take a random branch, we have to update the map according to the
    // new rankings of non-visited statements.
    void updateBufferRankings();
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_CORE_EXPLORATION_STRATEGY_RANDOM_ACCESS_MAX_COVERAGE_H_ */
