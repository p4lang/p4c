#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_FINAL_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_FINAL_STATE_H_

#include <functional>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/solver.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"
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
    std::reference_wrapper<const Model> finalModel;

    /// The final program trace.
    std::vector<std::reference_wrapper<const TraceEvent>> trace;

    /// If the calculated payload size (the size of the symbolic packet size variable minus the size
    /// of the input packet) is not negative, create a randomly sized payload and add the variable
    /// to the model. Only do this if the payload has not been set yet.
    static void calculatePayload(const ExecutionState &executionState, Model &evaluatedModel);

    /// Perform post processing on the final model and add custom sumbolic variables,
    /// which may not be part of the symbolic executor's model.
    /// For example, the payload is added once we have calculated the appropriate packet size.
    static Model &processModel(const ExecutionState &finalState, Model &model,
                               bool postProcess = true);

 public:
    /// This constructor invokes @ref processModel() to produce the model based on the solver
    /// and the executionState.
    FinalState(AbstractSolver &solver, const ExecutionState &finalState);

    /// This constructor takes the input model as is and does not invoke @ref processModel().
    FinalState(AbstractSolver &solver, const ExecutionState &finalState, const Model &finalModel);

    /// If there are concolic variables in the program, compute a new final state by rerunning the
    /// solver on the concolic assignments. If the concolic assignment is not satisfiable, return
    /// std::nullopt. Otherwise, create a new final state with the new assignment. IMPORTANT: Some
    /// variables in this final state may have been added in post, e.g., the payload size. If the
    /// concolic variables do not recompute these variables, the model will simply copy these
    /// variables over manually to the newly generated model.
    [[nodiscard]] std::optional<std::reference_wrapper<const FinalState>> computeConcolicState(
        const ConcolicVariableMap &resolvedConcolicVariables) const;

    /// @returns the model after it was augmented by completions from the symbolic environment.
    [[nodiscard]] const Model &getFinalModel() const;

    /// @returns the solver associated with this final state.
    [[nodiscard]] AbstractSolver &getSolver() const;

    /// @returns the execution state of this final state.
    [[nodiscard]] const ExecutionState *getExecutionState() const;

    /// @returns the computed traces of this final state.
    [[nodiscard]] const std::vector<std::reference_wrapper<const TraceEvent>> *getTraces() const;

    /// @returns the list of visited nodes of this state.
    [[nodiscard]] const P4::Coverage::CoverageSet &getVisited() const;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_FINAL_STATE_H_ */
