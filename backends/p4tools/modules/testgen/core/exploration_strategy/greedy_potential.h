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

namespace P4Tools::P4Testgen {

/// Greedy path selection strategy, which, at branch points, picks the first execution state which
/// has covered or potentially will cover program statements which have not been visited yet.
/// Potential statements are computed using the CollectLatentStatements visitor, which collects
/// statements in the top-level statement of the execution state. These statements are latent
/// because execution is not guaranteed. They may be guarded by an if condition or select
/// expression. If the strategy does not find a new statement, it falls back to
/// random. Similarly, if the strategy cycles without a test for a specific threshold, it will
/// fall back to random. This is to prevent getting caught in a parser cycle.
class GreedyPotential : public ExplorationStrategy {
 public:
    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    void run(const Callback &callBack) override;

    /// Constructor for this strategy, considering inheritance
    GreedyPotential(AbstractSolver &solver, const ProgramInfo &programInfo);

 private:
    /// This variable keeps track of how many branch decisions we have made without producing a
    /// test. This is a safety guard in case the strategy gets stuck in parser loops because of its
    /// greediness.
    uint64_t stepsWithoutTest = 0;

    /// The maximum number of steps without generating a test before falling back to random.
    static const uint64_t MAX_STEPS_WITHOUT_TEST = 1000;

    /// Branches which have the potential to cover new statements in the program.
    /// Each element on this vector represents a set of alternative choices that could have been
    /// made along the current execution path.
    ///
    /// Invariants:
    ///   - Each element of this vector is non-empty.
    ///   - Each element's path constraints are satisfiable.
    std::vector<Branch> potentialBranches;

    /// General unexplored branches.
    // Each element on this vector represents a set of alternative choices that could have been
    /// made along the current execution path.
    ///
    /// Invariants:
    ///   - Each element of this vector is non-empty.
    ///   - Each element's path constraints are satisfiable.
    ///   - There are no statements associated with the element's execution state that are
    ///   uncovered.
    std::vector<Branch> unexploredBranches;

    /// Take a branch and a solver as input.
    /// Compute the branch's path conditions using the solver.
    /// Return true if the solver can find a solution and does not time out.
    static bool evaluateBranch(const ExplorationStrategy::Branch &branch, AbstractSolver &solver);

    /// Iterate over all the input branches in @param candidateBranches and try to find a branch
    /// which contains statements that are not in @param coveredStatements yet. Return the first
    /// branch that was found and remove that branch from the container of @param candidateBranches.
    /// Return none, if no branch was found.
    static std::optional<ExplorationStrategy::Branch> popPotentialBranch(
        const P4::Coverage::CoverageSet &coveredStatements,
        std::vector<ExplorationStrategy::Branch> &candidateBranches);

    /// Select a branch at random from the input @param candidateBranches.
    //  Remove the branch from the container.
    static ExplorationStrategy::Branch popRandomBranch(
        std::vector<ExplorationStrategy::Branch> &candidateBranches);

    /// Try to pick a successor from the list of given successors. This involves three steps.
    /// 1. Filter out all the successors with unsatisfiable path conditions. If no successors are
    /// left, return false.
    /// 2. If there are successors left, try to find a successor that covers new statements. Set the
    /// nextState as this successors state.
    /// 3. If no successor with new statements was found set a random successor.
    [[nodiscard]] bool pickSuccessor(StepResult successors);
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_GREEDY_POTENTIAL_H_ */
