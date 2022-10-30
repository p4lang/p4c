#include "backends/p4tools/testgen/lib/final_state.h"

#include <vector>

#include "backends/p4tools/common/lib/symbolic_env.h"

namespace P4Tools {

namespace P4Testgen {

FinalState::FinalState(AbstractSolver* solver, const ExecutionState& inputState)
    : solver(solver),
      state(ExecutionState(inputState)),
      completedModel(completeModel(inputState, solver->getModel())) {
    for (const auto& event : inputState.getTrace()) {
        trace.emplace_back(event->evaluate(completedModel));
    }
    visitedStatements = inputState.getVisited();
}

Model FinalState::completeModel(const ExecutionState& executionState, const Model* model) {
    // Complete the model based on the symbolic environment.
    auto* completedModel = executionState.getSymbolicEnv().complete(*model);

    // Also complete all the zombies that were collected in this state.
    const auto& zombies = executionState.getZombies();
    completedModel->complete(zombies);

    // Now that the models initial values are completed evaluate the values that
    // are part of the constraints that have been added to the solver.
    auto* evaluatedModel = executionState.getSymbolicEnv().evaluate(*completedModel);

    for (const auto& event : executionState.getTrace()) {
        event->complete(evaluatedModel);
    }

    return *evaluatedModel;
}

const Model* FinalState::getCompletedModel() const { return &completedModel; }

AbstractSolver* FinalState::getSolver() const { return solver; }

const ExecutionState* FinalState::getExecutionState() const { return &state; }

const std::vector<gsl::not_null<const TraceEvent*>>* FinalState::getTraces() const {
    return &trace;
}

const std::vector<const IR::Statement*>& FinalState::getVisited() const {
    return visitedStatements;
}

}  // namespace P4Testgen

}  // namespace P4Tools
