#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_EXPR_STEPPER_H_

#include "ir/ir.h"
#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4::P4Tools::P4Testgen::Pna {

class SharedPnaExprStepper : public ExprStepper {
 private:
    // Provides implementations of common PNA externs.
    static const ExternMethodImpls<SharedPnaExprStepper> PNA_EXTERN_METHOD_IMPLS;

 public:
    SharedPnaExprStepper(ExecutionState &state, AbstractSolver &solver,
                         const ProgramInfo &programInfo);

    void evalExternMethodCall(const ExternInfo &externInfo) override;

    bool preorder(const IR::P4Table * /*table*/) override;
};
}  // namespace P4::P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_EXPR_STEPPER_H_ */
