#include "backends/p4tools/modules/testgen/lib/final_state.h"

#include <list>
#include <utility>
#include <variant>
#include <vector>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/error.h"
#include "lib/null.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"

namespace P4Tools::P4Testgen {

FinalState::FinalState(AbstractSolver &solver, const ExecutionState &finalState)
    : solver(solver),
      state(finalState),
      completedModel(completeModel(finalState, new Model(solver.getSymbolicMapping()))) {
    for (const auto &event : finalState.getTrace()) {
        trace.emplace_back(*event.get().evaluate(completedModel, true));
    }
}

FinalState::FinalState(AbstractSolver &solver, const ExecutionState &finalState,
                       const Model &completedModel)
    : solver(solver), state(finalState), completedModel(completedModel) {
    for (const auto &event : finalState.getTrace()) {
        trace.emplace_back(*event.get().evaluate(completedModel, true));
    }
}

void FinalState::calculatePayload(const ExecutionState &executionState, Model &evaluatedModel) {
    const auto &packetBitSizeVar = ExecutionState::getInputPacketSizeVar();
    const auto *payloadSizeConst = evaluatedModel.evaluate(packetBitSizeVar, true);
    int calculatedPacketSize = IR::getIntFromLiteral(payloadSizeConst);
    const auto *inputPacketExpr = executionState.getInputPacket();
    int payloadSize = calculatedPacketSize - inputPacketExpr->type->width_bits();
    if (payloadSize > 0) {
        const auto *payloadType = IR::getBitType(payloadSize);
        const IR::Expression *payloadExpr = evaluatedModel.get(&PacketVars::PAYLOAD_LABEL, false);
        if (payloadExpr == nullptr) {
            payloadExpr = Utils::getRandConstantForType(payloadType);
            evaluatedModel.emplace(&PacketVars::PAYLOAD_LABEL, payloadExpr);
        }
    }
}

Model &FinalState::completeModel(const ExecutionState &finalState, const Model *model,
                                 bool postProcess) {
    // Now that the models initial values are completed evaluate the values that
    // are part of the constraints that have been added to the solver.
    auto *evaluatedModel = finalState.getSymbolicEnv().evaluate(*model);

    if (postProcess) {
        // Append a payload, if requested.
        calculatePayload(finalState, *evaluatedModel);
    }

    return *evaluatedModel;
}

std::optional<std::reference_wrapper<const FinalState>> FinalState::computeConcolicState(
    const ConcolicVariableMap &resolvedConcolicVariables) const {
    // If there are no new concolic variables, there is nothing to do.
    if (resolvedConcolicVariables.empty()) {
        return *this;
    }
    std::vector<const Constraint *> asserts = state.get().getPathConstraint();

    for (const auto &resolvedConcolicVariable : resolvedConcolicVariables) {
        const auto &concolicVariable = resolvedConcolicVariable.first;
        const auto *concolicAssignment = resolvedConcolicVariable.second;
        const IR::Expression *pathConstraint = nullptr;
        // We need to differentiate between state variables and expressions here.
        if (std::holds_alternative<IR::ConcolicVariable>(concolicVariable)) {
            pathConstraint = new IR::Equ(std::get<IR::ConcolicVariable>(concolicVariable).clone(),
                                         concolicAssignment);
        } else if (std::holds_alternative<const IR::Expression *>(concolicVariable)) {
            pathConstraint =
                new IR::Equ(std::get<const IR::Expression *>(concolicVariable), concolicAssignment);
        }
        CHECK_NULL(pathConstraint);
        pathConstraint = state.get().getSymbolicEnv().subst(pathConstraint);
        pathConstraint = P4::optimizeExpression(pathConstraint);
        asserts.push_back(pathConstraint);
    }
    auto solverResult = solver.get().checkSat(asserts);
    if (!solverResult) {
        ::warning("Timed out trying to solve this concolic execution path.");
        return std::nullopt;
    }

    if (!*solverResult) {
        ::warning("Concolic constraints for this path are unsatisfiable.");
        return std::nullopt;
    }
    auto &model = completeModel(state, new Model(solver.get().getSymbolicMapping()), false);
    /// Transfer any derived variables from that are missing  in this model.
    /// Do NOT update any variables that already exist.
    for (const auto &varTuple : completedModel.get()) {
        model.emplace(varTuple.first, varTuple.second);
    }
    return *new FinalState(solver, state, model);
}

const Model &FinalState::getCompletedModel() const { return completedModel; }

AbstractSolver &FinalState::getSolver() const { return solver; }

const ExecutionState *FinalState::getExecutionState() const { return &state.get(); }

const std::vector<std::reference_wrapper<const TraceEvent>> *FinalState::getTraces() const {
    return &trace;
}

const P4::Coverage::CoverageSet &FinalState::getVisited() const { return state.get().getVisited(); }
}  // namespace P4Tools::P4Testgen
