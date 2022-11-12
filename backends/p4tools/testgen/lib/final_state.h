#ifndef BACKENDS_P4TOOLS_TESTGEN_LIB_FINAL_STATE_H_
#define BACKENDS_P4TOOLS_TESTGEN_LIB_FINAL_STATE_H_

#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"

#include "backends/p4tools/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

/// Represents the final state after execution.
class FinalState {
 private:
    /// The solver associated with this final state. We need the solver to resolve  under new
    /// constraints in the case of concolic execution.
    gsl::not_null<AbstractSolver*> solver;
    /// The final state of the execution.
    const ExecutionState state;
    /// The final model which has been augmented with environment completions.
    const Model completedModel;
    /// The final program trace.
    std::vector<gsl::not_null<const TraceEvent*>> trace;
    /// The final list of visited statements.
    std::vector<const IR::Statement*> visitedStatements;

 public:
    FinalState(AbstractSolver* solver, const ExecutionState& inputState);

    /// Complete the model according to target-specific completion criteria.
    /// We first complete (this means we fill all the variables that have not been bound).
    /// Then we evaluate the model (we assign values to the variables that have been bound).
    static Model completeModel(const ExecutionState& executionState, const Model* model);

    /// @returns the model after it was augmented by completions from the symbolic environment.
    const Model* getCompletedModel() const;

    /// @returns the solver associated with this final state.
    AbstractSolver* getSolver() const;

    /// @returns the execution state of this final state.
    const ExecutionState* getExecutionState() const;

    /// @returns the computed traces of this final state.
    const std::vector<gsl::not_null<const TraceEvent*>>* getTraces() const;

    /// @returns the list of visited statements.
    const std::vector<const IR::Statement*>& getVisited() const;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_LIB_FINAL_STATE_H_ */
