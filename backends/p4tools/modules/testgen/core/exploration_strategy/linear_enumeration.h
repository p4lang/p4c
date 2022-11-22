#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_LINEAR_ENUMERATION_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_LINEAR_ENUMERATION_H_

#include <cstdint>
#include <vector>

#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"

namespace P4Tools {

namespace P4Testgen {

/// Visits all possible explorable branches and stores them in a list,
/// which will be picked randomly to produce a test. This feature is still
/// *experimetnal* and it can take a long time to produce tests depending on
// the chosen max bound.
class LinearEnumeration : public ExplorationStrategy {
 public:
    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    void run(const Callback& callBack) override;

    /// Constructor for this strategy, considering inheritance.
    LinearEnumeration(AbstractSolver& solver, const ProgramInfo& programInfo,
                      boost::optional<uint32_t> seed, int linearEnumeration);

 protected:
    /// The max size for the exploredBranches vector.
    uint64_t maxBound;

    /// A vector, wherein each element (a Branch) is from a terminal state, and therefore
    /// can generate a test case.
    std::vector<Branch> exploredBranches;

    /// Receives a branch and an evaluator, checks if the branch has a next state, then
    /// recursively steps into the states, gets the successors and calls itself again.
    ///
    /// It will recursively map all terminal states, or at least fill the exploredBranches
    /// vector until it reaches the maxBoud.
    void mapBranch(Branch& branch);
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_LINEAR_ENUMERATION_H_ */
