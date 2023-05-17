#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_FINAL_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_FINAL_STATE_H_

#include <functional>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// Represents the final state after execution.
class FinalState {
 private:
    /// The solver associated with this final state. We need the solver to resolve  under new
    /// constraints in the case of concolic execution.
    std::reference_wrapper<AbstractSolver> solver;

    /// The final state of the execution.
    std::reference_wrapper<const ExecutionState> state;

    /// The final model which has been augmented with environment completions.
    const Model completedModel;

    /// The final program trace.
    std::vector<std::reference_wrapper<const TraceEvent>> trace;

 public:
    FinalState(AbstractSolver &solver, const ExecutionState &inputState);

    /// Complete the model according to target-specific completion criteria.
    /// We first complete (this means we fill all the variables that have not been bound).
    /// Then we evaluate the model (we assign values to the variables that have been bound).
    static Model completeModel(const ExecutionState &executionState, const Model *model);

    /// @returns the model after it was augmented by completions from the symbolic environment.
    [[nodiscard]] const Model *getCompletedModel() const;

    /// @returns the solver associated with this final state.
    [[nodiscard]] AbstractSolver &getSolver() const;

    /// @returns the execution state of this final state.
    [[nodiscard]] const ExecutionState *getExecutionState() const;

    /// @returns the computed traces of this final state.
    [[nodiscard]] const std::vector<std::reference_wrapper<const TraceEvent>> *getTraces() const;

    /// @returns the list of visited statements of this state.
    [[nodiscard]] const P4::Coverage::CoverageSet &getVisited() const;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_FINAL_STATE_H_ */
