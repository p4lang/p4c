#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_UNBOUNDED_RANDOM_ACCESS_STACK_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_UNBOUNDED_RANDOM_ACCESS_STACK_H_

#include <cstdint>
#include <list>

#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/incremental_stack.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"

namespace P4Tools {

namespace P4Testgen {

/// Another stack-based exploration strategy; this time we enable random
/// access to different points of the stack via the multiPop method.
/// Param popLevel specifies how deep we can pop the levels of stack, and
/// it controls the amount of randomness we insert.
class UnboundedRandomAccessStack : public IncrementalStack {
 public:
    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    void run(const Callback &callBack) override;

    /// Constructor for this strategy, considering inheritance
    UnboundedRandomAccessStack(AbstractSolver &solver, const ProgramInfo &programInfo);

 private:
    // Buffer of unexploredBranches. It saves the unexplored branches,
    // so we can restore them if multiPop empties the current unexploredBranches
    std::list<StepResult> bufferUnexploredBranches;

    // pops multiple levels of an unexploredBranch and handles buffer updates
    StepResult multiPop(UnexploredBranches &unexploredBranches);
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_CORE_EXPLORATION_STRATEGY_UNBOUNDED_RANDOM_ACCESS_STACK_H_ */
