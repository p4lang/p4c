#ifndef TESTGEN_CORE_EXPLORATION_STRATEGY_EXPLORATION_STRATEGY_H_
#define TESTGEN_CORE_EXPLORATION_STRATEGY_EXPLORATION_STRATEGY_H_

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

#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "gsl/gsl-lite.hpp"

#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/core/small_step/small_step.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/lib/final_state.h"

namespace P4Tools {

namespace P4Testgen {

/// Base abstract class for exploration strategy. It requires the implementation of
/// the run method, and carries the base Branch struct, to be reused in inherited
/// classes. It also holds a default termination method, which can be override.
class ExplorationStrategy {
 public:
    /// Callbacks are invoked when the P4 program terminates. If the callback returns true,
    /// execution halts. Otherwise, execution of the P4 program continues on a different random
    /// path.
    using Callback = std::function<bool(const FinalState&)>;

    using Branch = SmallStepEvaluator::Branch;
    using StepResult = SmallStepEvaluator::Result;

    /// Executes the P4 program along a randomly chosen path. When the program terminates, the
    /// given callback is invoked. If the callback returns true, then the executor terminates.
    /// Otherwise, execution of the P4 program continues on a different random path.
    /// TODO there is a lot of code repetition in subclasses. Refactor and extract duplicates.
    virtual void run(const Callback& callBack) = 0;

    explicit ExplorationStrategy(AbstractSolver& solver, const ProgramInfo& programInfo,
                                 boost::optional<uint32_t> seed);

    /// Writes a list of the selected branches into @param out.
    void printCurrentTraceAndBranches(std::ostream& out);

 protected:
    /// Target-specific information about the P4 program.
    const ProgramInfo& programInfo;

    /// The SMT solver backing this executor.
    AbstractSolver& solver;

    /// @returns a pseudorandom integer in the range of [0, max]
    uint64_t selectBranch(const std::vector<Branch>& branches);

    /// Handles processing at the end of a P4 program.
    ///
    /// @returns true if symbolic execution should end; false if symbolic execution should continue
    /// on a different path.
    bool handleTerminalState(const Callback& callback, const ExecutionState& terminalState);

    /// Take one step in the program and return list of possible branches.
    StepResult step(ExecutionState& state);

    /// The current execution state.
    ExecutionState* executionState = nullptr;

 private:
    SmallStepEvaluator evaluator;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_CORE_EXPLORATION_STRATEGY_EXPLORATION_STRATEGY_H_ */
