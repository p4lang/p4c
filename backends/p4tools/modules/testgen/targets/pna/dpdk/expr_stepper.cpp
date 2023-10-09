#include "backends/p4tools/modules/testgen/targets/pna/dpdk/expr_stepper.h"

#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/externs.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/table_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_expr_stepper.h"

namespace P4Tools::P4Testgen::Pna {

std::string PnaDpdkExprStepper::getClassName() { return "PnaDpdkExprStepper"; }

PnaDpdkExprStepper::PnaDpdkExprStepper(ExecutionState &state, AbstractSolver &solver,
                                       const ProgramInfo &programInfo)
    : SharedPnaExprStepper(state, solver, programInfo) {}

// Provides implementations of PNA-DPDK externs.
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
const ExternMethodImpls PnaDpdkExprStepper::PNA_DPDK_EXTERN_METHOD_IMPLS({});

void PnaDpdkExprStepper::evalExternMethodCall(const IR::MethodCallExpression *call,
                                              const IR::Expression *receiver, IR::ID name,
                                              const IR::Vector<IR::Argument> *args,
                                              ExecutionState &state) {
    if (!PNA_DPDK_EXTERN_METHOD_IMPLS.exec(call, receiver, name, args, state, result)) {
        SharedPnaExprStepper::evalExternMethodCall(call, receiver, name, args, state);
    }
}

bool PnaDpdkExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    PnaDpdkTableStepper tableStepper(this, table);
    return PnaDpdkTableStepper(this, table).eval();
}

}  // namespace P4Tools::P4Testgen::Pna
