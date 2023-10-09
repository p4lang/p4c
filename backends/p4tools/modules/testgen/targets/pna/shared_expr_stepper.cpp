#include "backends/p4tools/modules/testgen/targets/pna/shared_expr_stepper.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"
#include "ir/solver.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/core/externs.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_table_stepper.h"

namespace P4Tools::P4Testgen::Pna {

SharedPnaExprStepper::SharedPnaExprStepper(ExecutionState &state, AbstractSolver &solver,
                                           const ProgramInfo &programInfo)
    : ExprStepper(state, solver, programInfo) {}

// Provides implementations of common PNA externs.
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
const ExternMethodImpls SharedPnaExprStepper::PNA_EXTERN_METHOD_IMPLS({
    /* ======================================================================================
     *  drop_packet
     *  drop_packet sets the PNA internal drop variable to true.
     * ====================================================================================== */
    {"*method.drop_packet",
     {},
     [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
        IR::ID & /*methodName*/, const IR::Vector<IR::Argument> * /*args*/,
        const ExecutionState &state, SmallStepEvaluator::Result &result) {
         auto &nextState = state.clone();
         // Use an assignment to set drop variable to true.
         // This variable will be processed in the deparser.
         nextState.set(&PnaConstants::DROP_VAR, IR::getBoolLiteral(true));
         nextState.add(*new TraceEvents::Generic("drop_packet executed"));
         nextState.popBody();
         result->emplace_back(nextState);
     }},
    /* ======================================================================================
     *  send_to_port
     *  send_to_port sets the PNA internal egress port variable to true.
     * ====================================================================================== */
    // TODO: Implement extern path expression calls.
    {"*method.send_to_port",
     {"dest_port"},
     [](const IR::MethodCallExpression *call, const IR::Expression * /*receiver*/,
        IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args, const ExecutionState &state,
        SmallStepEvaluator::Result &result) {
         auto &nextState = state.clone();

         const auto *destPort = args->at(0)->expression;
         // TODO: Frontload this in the expression stepper for method call expressions.
         if (!SymbolicEnv::isSymbolicValue(destPort)) {
             // Evaluate the condition.
             stepToSubexpr(destPort, result, state, [call](const Continuation::Parameter *v) {
                 auto *clonedCall = call->clone();
                 auto *arguments = clonedCall->arguments->clone();
                 auto *arg = arguments->at(0)->clone();
                 arg->expression = v->param;
                 (*arguments)[0] = arg;
                 clonedCall->arguments = arguments;
                 return Continuation::Return(clonedCall);
             });
             return;
         }
         // Use an assignment to set the output port to the input.
         nextState.set(&PnaConstants::OUTPUT_PORT_VAR, destPort);
         nextState.add(*new TraceEvents::Generic("send_to_port executed"));
         nextState.popBody();
         result->emplace_back(nextState);
     }},
    /* ======================================================================================
     *  SelectByDirection
     *  SelectByDirection is a simple pure function that behaves exactly as
     *  the P4_16 function definition given in comments below.  It is an
     *  extern function to ensure that the front/mid end of the p4c
     *  compiler leaves occurrences of it as is, visible to target-specific
     *  compiler back end code, so targets have all information needed to
     *  optimize it as they wish.
     *  One example of its use is in table key expressions, for tables
     *  where one wishes to swap IP source/destination addresses for
     *  packets processed in the different directions.
     *  T SelectByDirection<T>(
     *     in PNA_Direction_t direction,
     *     in T n2h_value,
     *     in T h2n_value)
     *  {
     *     if (direction == PNA_Direction_t.NET_TO_HOST) {
     *         return n2h_value;
     *     } else {
     *         return h2n_value;
     *     }
     * }
     * ====================================================================================== */
    {"*method.SelectByDirection",
     {"direction", "n2h_value", "h2n_value"},
     [](const IR::MethodCallExpression *call, const IR::Expression * /*receiver*/,
        IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args, const ExecutionState &state,
        SmallStepEvaluator::Result &result) {
         for (size_t idx = 0; idx < args->size(); ++idx) {
             const auto *arg = args->at(idx);
             const auto *argExpr = arg->expression;

             // TODO: Frontload this in the expression stepper for method call expressions.
             if (!SymbolicEnv::isSymbolicValue(argExpr)) {
                 // Evaluate the condition.
                 stepToSubexpr(argExpr, result, state,
                               [call, idx](const Continuation::Parameter *v) {
                                   auto *clonedCall = call->clone();
                                   auto *arguments = clonedCall->arguments->clone();
                                   auto *arg = arguments->at(idx)->clone();
                                   arg->expression = v->param;
                                   (*arguments)[idx] = arg;
                                   clonedCall->arguments = arguments;
                                   return Continuation::Return(clonedCall);
                               });
                 return;
             }
         }
         const auto *directionVar = args->at(0)->expression;
         const auto *n2hValue = args->at(1)->expression;
         const auto *h2nValue = args->at(2)->expression;
         if (Taint::hasTaint(directionVar)) {
             auto &taintedState = state.clone();
             taintedState.replaceTopBody(
                 Continuation::Return(ToolsVariables::getTaintExpression(n2hValue->type)));
             result->emplace_back(taintedState);
             return;
         };
         auto *cond = new IR::Equ(
             directionVar, IR::getConstant(directionVar->type, PacketDirection::NET_TO_HOST));

         // Return the NET_TO_HOST value.
         auto &netToHostState = state.clone();
         netToHostState.replaceTopBody(Continuation::Return(n2hValue));
         result->emplace_back(cond, state, netToHostState);

         // Return the HOST_TO_NET value.
         auto &hostToNetState = state.clone();
         hostToNetState.replaceTopBody(Continuation::Return(h2nValue));
         result->emplace_back(new IR::LNot(cond), state, netToHostState);
     }},
    /* ======================================================================================
     *  add_entry
     *  add_entry adds a new entry to the table whose default action calls
     * the add_entry function. The new entry will have the same key that was just looked up.
     * ====================================================================================== */
    // TODO: Currently, this is a no-op. This is because we only test a single packet input.
    // Implement a way to test add_entry.
    {"*method.add_entry",
     {"action_name", "action_params", "expire_time_profile_id"},
     [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
        IR::ID & /*methodName*/, const IR::Vector<IR::Argument> * /*args*/,
        const ExecutionState &state, SmallStepEvaluator::Result &result) {
         auto &nextState = state.clone();
         nextState.add(*new TraceEvents::Generic("add_entry executed. Currently a no-op."));
         nextState.replaceTopBody(Continuation::Return(IR::getBoolLiteral(true)));
         result->emplace_back(nextState);
     }},
});

void SharedPnaExprStepper::evalExternMethodCall(const IR::MethodCallExpression *call,
                                                const IR::Expression *receiver, IR::ID name,
                                                const IR::Vector<IR::Argument> *args,
                                                ExecutionState &state) {
    if (!PNA_EXTERN_METHOD_IMPLS.exec(call, receiver, name, args, state, result)) {
        ExprStepper::evalExternMethodCall(call, receiver, name, args, state);
    }
}

bool SharedPnaExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    SharedPnaTableStepper tableStepper(this, table);

    return tableStepper.eval();
}

}  // namespace P4Tools::P4Testgen::Pna
