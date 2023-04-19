#include "backends/p4tools/modules/testgen/lib/final_state.h"

#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_event.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

FinalState::FinalState(AbstractSolver &solver, const ExecutionState &inputState)
    : solver(solver),
      state(inputState),
      completedModel(completeModel(inputState, new Model(solver.getSymbolicMapping()))) {
    for (const auto &event : inputState.getTrace()) {
        trace.emplace_back(*event.get().evaluate(completedModel));
    }
}

Model FinalState::completeModel(const ExecutionState &executionState, const Model *model) {
    // Complete the model based on the symbolic environment.
    auto *completedModel = executionState.getSymbolicEnv().complete(*model);

    // Also complete all the zombies that were collected in this state.
    const auto &zombies = executionState.getSymbolicVariables();
    completedModel->complete(zombies);

    // Now that the models initial values are completed evaluate the values that
    // are part of the constraints that have been added to the solver.
    auto *evaluatedModel = executionState.getSymbolicEnv().evaluate(*completedModel);

    for (const auto &event : executionState.getTrace()) {
        event.get().complete(evaluatedModel);
    }
    return *evaluatedModel;
}

const Model *FinalState::getCompletedModel() const { return &completedModel; }

AbstractSolver &FinalState::getSolver() const { return solver; }

const ExecutionState *FinalState::getExecutionState() const { return &state.get(); }

const std::vector<std::reference_wrapper<const TraceEvent>> *FinalState::getTraces() const {
    return &trace;
}

const P4::Coverage::CoverageSet &FinalState::getVisited() const { return state.get().getVisited(); }

}  // namespace P4Tools::P4Testgen
