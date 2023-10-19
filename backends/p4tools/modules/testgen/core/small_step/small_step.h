#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_SMALL_STEP_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_SMALL_STEP_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "backends/p4tools/common/compiler/reachability.h"
#include "ir/node.h"
#include "ir/solver.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// The main class that implements small-step operational semantics. Delegates to implementations
/// of AbstractStepper.
class SmallStepEvaluator {
    friend class CommandVisitor;

 public:
    /// A branch is an execution state paired with an optional path constraint representing the
    /// choice made to take the branch.
    struct Branch {
        const Constraint *constraint;

        ExecutionStateReference nextState;

        P4::Coverage::CoverageSet potentialNodes;

        /// Simple branch without any constraint.
        explicit Branch(ExecutionState &nextState);

        /// Branch constrained by a condition. prevState is the state in which the condition
        /// is later evaluated.
        Branch(std::optional<const Constraint *> c, const ExecutionState &prevState,
               ExecutionState &nextState);

        /// Branch constrained by a condition. prevState is the state in which the condition
        /// is later evaluated.
        Branch(std::optional<const Constraint *> c, const ExecutionState &prevState,
               ExecutionState &nextState, P4::Coverage::CoverageSet potentialNodes);
    };

    using Result = std::vector<Branch> *;

    /// Specifies how many times a guard can be violated in the interpreter until it throws an
    /// error.
    static constexpr uint64_t MAX_GUARD_VIOLATIONS = 100;

 private:
    /// Target-specific information about the P4 program being evaluated.
    const ProgramInfo &programInfo;

    /// The solver backing this evaluator.
    AbstractSolver &solver;

    /// The number of times a guard was not satisfiable.
    uint64_t violatedGuardConditions = 0;

    /// Reachability engine.
    ReachabilityEngine *reachabilityEngine = nullptr;

    using REngineType = std::pair<ReachabilityResult, std::vector<SmallStepEvaluator::Branch> *>;

    static void renginePostprocessing(ReachabilityResult &result,
                                      std::vector<SmallStepEvaluator::Branch> *branches);

    REngineType renginePreprocessing(SmallStepEvaluator &stepper, const ExecutionState &nextState,
                                     const IR::Node *node);

 public:
    Result step(ExecutionState &state);

    SmallStepEvaluator(AbstractSolver &solver, const ProgramInfo &programInfo);
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_SMALL_STEP_H_ */
