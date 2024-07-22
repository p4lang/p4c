#include "backends/p4tools/modules/testgen/targets/pna/dpdk/expr_stepper.h"

#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/table_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_expr_stepper.h"

namespace P4C::P4Tools::P4Testgen::Pna {

std::string PnaDpdkExprStepper::getClassName() { return "PnaDpdkExprStepper"; }

PnaDpdkExprStepper::PnaDpdkExprStepper(ExecutionState &state, AbstractSolver &solver,
                                       const ProgramInfo &programInfo)
    : SharedPnaExprStepper(state, solver, programInfo) {}

// Provides implementations of PNA-DPDK externs.
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
const ExprStepper::ExternMethodImpls<PnaDpdkExprStepper>
    PnaDpdkExprStepper::PNA_DPDK_EXTERN_METHOD_IMPLS({});

void PnaDpdkExprStepper::evalExternMethodCall(const ExternInfo &externInfo) {
// Remove this once an extern is implemented.
#if 0
    auto method = PNA_DPDK_EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArguments);
    if (method.has_value()) {
        return method.value()(externInfo, *this);
    }
#endif
    // Lastly, check whether we are calling an internal extern method.
    return SharedPnaExprStepper::evalExternMethodCall(externInfo);
}

bool PnaDpdkExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    PnaDpdkTableStepper tableStepper(this, table);
    return PnaDpdkTableStepper(this, table).eval();
}

}  // namespace P4C::P4Tools::P4Testgen::Pna
