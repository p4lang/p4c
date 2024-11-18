#include "backends/p4tools/modules/testgen/targets/pna/p4tc/expr_stepper.h"

#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/p4tc/table_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_expr_stepper.h"

namespace P4::P4Tools::P4Testgen::Pna {

std::string PnaP4TCExprStepper::getClassName() { return "PnaP4TCExprStepper"; }

PnaP4TCExprStepper::PnaP4TCExprStepper(ExecutionState &state, AbstractSolver &solver,
                                       const ProgramInfo &programInfo)
    : SharedPnaExprStepper(state, solver, programInfo) {}

// Provides implementations of PNA-DPDK externs.
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
const ExprStepper::ExternMethodImpls<PnaP4TCExprStepper>
    PnaP4TCExprStepper::PNA_DPDK_EXTERN_METHOD_IMPLS({});

void PnaP4TCExprStepper::evalExternMethodCall(const ExternInfo &externInfo) {
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

bool PnaP4TCExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    PnaP4TCTableStepper tableStepper(this, table);
    return PnaP4TCTableStepper(this, table).eval();
}

}  // namespace P4::P4Tools::P4Testgen::Pna
