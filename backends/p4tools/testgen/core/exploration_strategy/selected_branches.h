#ifndef TESTGEN_CORE_EXPLORATION_STRATEGY_SELECTED_BRANCHES_H_
#define TESTGEN_CORE_EXPLORATION_STRATEGY_SELECTED_BRANCHES_H_

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

/// Explores one path described by a list of branches.
class SelectedBranches : public ExplorationStrategy {
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
    SelectedBranches(AbstractSolver& solver, const ProgramInfo& programInfo,
                     boost::optional<uint32_t> seed, std::string selectedBranchesStr);

 private:
    /// Chooses a branch corresponding to a given branch identifier.
    ///
    /// @returns next execution state to be examined, throws an exception on invalid nextBranch.
    ExecutionState* chooseBranch(const std::vector<Branch>& branches, uint64_t nextBranch);

    /// The list of selected branches.
    std::list<uint64_t> selectedBranches;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_CORE_EXPLORATION_STRATEGY_SELECTED_BRANCHES_H_ */
