#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_

#include <stdint.h>

#include <string>

#include "backends/p4tools/common/core/solver.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/vector.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

class BMv2_V1ModelExprStepper : public ExprStepper {
 protected:
    std::string getClassName() override;

 private:
    // Helper function that checks whether the given structure filed has a 'field_list' annotation
    // and the recirculate index matches. @returns true if that is the case.
    static bool isPartOfFieldList(const IR::StructField *field, uint64_t recirculateIndex);

    /// This is a utility function for recirculation externs. This function resets all the values
    /// associated with @ref unless a value contained in the Type_StructLike type of ref has an
    /// annotation associated with it. If the annotation index matches @param recirculateIndex, the
    /// reference is not reset.
    void resetPreservingFieldList(ExecutionState *nextState, const IR::PathExpression *ref,
                                  uint64_t recirculateIndex) const;

 public:
    BMv2_V1ModelExprStepper(ExecutionState &state, AbstractSolver &solver,
                            const ProgramInfo &programInfo);

    void evalExternMethodCall(const IR::MethodCallExpression *call, const IR::Expression *receiver,
                              IR::ID name, const IR::Vector<IR::Argument> *args,
                              ExecutionState &state) override;

    bool preorder(const IR::P4Table * /*table*/) override;
};
}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_ */
