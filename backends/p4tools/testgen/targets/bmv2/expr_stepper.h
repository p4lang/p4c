#ifndef TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_
#define TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_

#include <string>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"

#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

class BMv2_V1ModelExprStepper : public ExprStepper {
 protected:
    std::string getClassName() override { return "BMv2_V1ModelExprStepper"; }

 public:
    BMv2_V1ModelExprStepper(ExecutionState& state, AbstractSolver& solver,
                            const ProgramInfo& programInfo);

    void evalExternMethodCall(const IR::MethodCallExpression* call, const IR::Expression* receiver,
                              IR::ID name, const IR::Vector<IR::Argument>* args,
                              ExecutionState& state) override;

    bool preorder(const IR::P4Table* /*table*/) override;
};
}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_ */
