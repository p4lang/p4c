#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_EXPR_STEPPER_H_

#include <string>

#include "ir/id.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "ir/vector.h"

#include "backends/p4tools/modules/testgen/core/externs.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_expr_stepper.h"

namespace P4Tools::P4Testgen::Pna {

class PnaDpdkExprStepper : public SharedPnaExprStepper {
 protected:
    std::string getClassName() override;

 private:
    // Provides implementations of PNA-DPDK externs.
    static const ExternMethodImpls PNA_DPDK_EXTERN_METHOD_IMPLS;

 public:
    PnaDpdkExprStepper(ExecutionState &state, AbstractSolver &solver,
                       const ProgramInfo &programInfo);

    void evalExternMethodCall(const IR::MethodCallExpression *call, const IR::Expression *receiver,
                              IR::ID name, const IR::Vector<IR::Argument> *args,
                              ExecutionState &state) override;

    bool preorder(const IR::P4Table * /*table*/) override;
};
}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_EXPR_STEPPER_H_ */
