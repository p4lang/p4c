#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_SMALL_STEP_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_SMALL_STEP_H_

#include <cstdint>
#include <vector>

#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "gsl/gsl-lite.hpp"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

/// The main class that implements small-step operational semantics. Delegates to implementations
/// of AbstractStepper.
class SmallStepEvaluator {
 public:
    /// A branch is an execution state paired with an optional path constraint representing the
    /// choice made to take the branch.
    struct Branch {
        const Constraint* constraint;

        gsl::not_null<ExecutionState*> nextState;

        /// Simple branch without any constraint.
        explicit Branch(gsl::not_null<ExecutionState*> nextState);

        /// Branch constrained by a condition. prevState is the state in which the condition
        /// is later evaluated.
        Branch(boost::optional<const Constraint*> c, const ExecutionState& prevState,
               gsl::not_null<ExecutionState*> nextState);
    };

    using Result = std::vector<Branch>*;

    /// Specifies how many times a guard can be violated in the interpreter until it throws an
    /// error.
    static constexpr uint64_t MAX_GUARD_VIOLATIONS = 100;

 private:
    /// Target-specific information about the P4 program being evaluated.
    const ProgramInfo& programInfo;

    /// The solver backing this evaluator.
    AbstractSolver& solver;

    /// The number of times a guard was not satisfiable.
    uint64_t violatedGuardConditions = 0;

    /// Reachability engine.
    ReachabilityEngine* reachabilityEngine = nullptr;

 public:
    Result step(ExecutionState& state);

    const IR::Expression* stepAndReturnValue(const IR::Expression* expr, ExecutionState& state);

    SmallStepEvaluator(AbstractSolver& solver, const ProgramInfo& programInfo);
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_SMALL_STEP_H_ */
