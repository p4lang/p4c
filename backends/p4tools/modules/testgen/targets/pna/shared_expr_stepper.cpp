#include "backends/p4tools/modules/testgen/targets/pna/shared_expr_stepper.h"

#include <cstddef>
#include <functional>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"
#include "ir/solver.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
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
const ExprStepper::ExternMethodImpls<SharedPnaExprStepper>
    SharedPnaExprStepper::PNA_EXTERN_METHOD_IMPLS({
        /* ======================================================================================
         *  drop_packet
         *  drop_packet sets the PNA internal drop variable to true.
         * ====================================================================================== */
        {"*method.drop_packet"_cs,
         {},
         [](const ExternInfo & /*externInfo*/, SharedPnaExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             // Use an assignment to set drop variable to true.
             // This variable will be processed in the deparser.
             nextState.set(&PnaConstants::DROP_VAR, IR::BoolLiteral::get(true));
             nextState.add(*new TraceEvents::Generic("drop_packet executed"_cs));
             nextState.popBody();
             stepper.result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  send_to_port
         *  send_to_port sets the PNA internal egress port variable to true.
         * ====================================================================================== */
        // TODO: Implement extern path expression calls.
        {"*method.send_to_port"_cs,
         {"dest_port"_cs},
         [](const ExternInfo &externInfo, SharedPnaExprStepper &stepper) {
             auto &nextState = stepper.state.clone();

             const auto *destPort = externInfo.externArguments.at(0)->expression;
             // TODO: Frontload this in the expression stepper for method call expressions.
             if (!SymbolicEnv::isSymbolicValue(destPort)) {
                 // Evaluate the condition.
                 stepToSubexpr(destPort, stepper.result, stepper.state,
                               [&externInfo](const Continuation::Parameter *v) {
                                   auto *clonedCall = externInfo.originalCall.clone();
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
             nextState.add(*new TraceEvents::Generic("send_to_port executed"_cs));
             nextState.popBody();
             stepper.result->emplace_back(nextState);
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
        {"*method.SelectByDirection"_cs,
         {"direction"_cs, "n2h_value"_cs, "h2n_value"_cs},
         [](const ExternInfo &externInfo, SharedPnaExprStepper &stepper) {
             for (size_t idx = 0; idx < externInfo.externArguments.size(); ++idx) {
                 const auto *arg = externInfo.externArguments.at(idx);
                 const auto *argExpr = arg->expression;

                 // TODO: Frontload this in the expression stepper for method call expressions.
                 if (!SymbolicEnv::isSymbolicValue(argExpr)) {
                     // Evaluate the condition.
                     stepToSubexpr(argExpr, stepper.result, stepper.state,
                                   [&externInfo, idx](const Continuation::Parameter *v) {
                                       auto *clonedCall = externInfo.originalCall.clone();
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
             const auto *directionVar = externInfo.externArguments.at(0)->expression;
             const auto *n2hValue = externInfo.externArguments.at(1)->expression;
             const auto *h2nValue = externInfo.externArguments.at(2)->expression;
             if (Taint::hasTaint(directionVar)) {
                 auto &taintedState = stepper.state.clone();
                 taintedState.replaceTopBody(
                     Continuation::Return(ToolsVariables::getTaintExpression(n2hValue->type)));
                 stepper.result->emplace_back(taintedState);
                 return;
             };
             auto *cond = new IR::Equ(
                 directionVar, IR::Constant::get(directionVar->type, PacketDirection::NET_TO_HOST));

             // Return the NET_TO_HOST value.
             auto &netToHostState = stepper.state.clone();
             netToHostState.replaceTopBody(Continuation::Return(n2hValue));
             stepper.result->emplace_back(cond, stepper.state, netToHostState);

             // Return the HOST_TO_NET value.
             auto &hostToNetState = stepper.state.clone();
             hostToNetState.replaceTopBody(Continuation::Return(h2nValue));
             stepper.result->emplace_back(new IR::LNot(cond), stepper.state, netToHostState);
         }},
        /* ======================================================================================
         *  add_entry
         *  add_entry adds a new entry to the table whose default action calls
         * the add_entry function. The new entry will have the same key that was just looked up.
         * ====================================================================================== */
        // TODO: Currently, this is a no-op. This is because we only test a single packet input.
        // Implement a way to test add_entry.
        {"*method.add_entry"_cs,
         {"action_name"_cs, "action_params"_cs, "expire_time_profile_id"_cs},
         [](const ExternInfo & /*externInfo*/, SharedPnaExprStepper &stepper) {
             auto &nextState = stepper.state.clone();
             nextState.add(*new TraceEvents::Generic("add_entry executed. Currently a no-op."_cs));
             nextState.replaceTopBody(Continuation::Return(IR::BoolLiteral::get(true)));
             stepper.result->emplace_back(nextState);
         }},
    });

void SharedPnaExprStepper::evalExternMethodCall(const ExternInfo &externInfo) {
    auto method = PNA_EXTERN_METHOD_IMPLS.find(externInfo.externObjectRef, externInfo.methodName,
                                               externInfo.externArguments);
    if (method.has_value()) {
        return method.value()(externInfo, *this);
    }
    // Lastly, check whether we are calling an internal extern method.
    return ExprStepper::evalExternMethodCall(externInfo);
}

bool SharedPnaExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    SharedPnaTableStepper tableStepper(this, table);

    return tableStepper.eval();
}

}  // namespace P4Tools::P4Testgen::Pna
