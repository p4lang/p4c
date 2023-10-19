#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_

#include <cstdint>
#include <string>

#include "ir/id.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "ir/vector.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen::Bmv2 {

class Bmv2V1ModelExprStepper : public ExprStepper {
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
    void resetPreservingFieldList(ExecutionState &nextState, const IR::PathExpression *ref,
                                  uint64_t recirculateIndex) const;

    /// Helper function, which is triggered when clone was called in the P4 program.
    void processClone(const ExecutionState &state, SmallStepEvaluator::Result &result);

    /// Helper function, which is triggered when resubmit or recirculate was called in the P4
    /// program.
    void processRecirculate(const ExecutionState &state, SmallStepEvaluator::Result &result);

 public:
    Bmv2V1ModelExprStepper(ExecutionState &state, AbstractSolver &solver,
                           const ProgramInfo &programInfo);

    void evalExternMethodCall(const IR::MethodCallExpression *call, const IR::Expression *receiver,
                              IR::ID name, const IR::Vector<IR::Argument> *args,
                              ExecutionState &state) override;

    bool preorder(const IR::P4Table * /*table*/) override;
};
}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_EXPR_STEPPER_H_ */
