#ifndef TESTGEN_TARGETS_EBPF_EXPR_STEPPER_H_
#define TESTGEN_TARGETS_EBPF_EXPR_STEPPER_H_

#include <string>

#include "ir/id.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "ir/vector.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen::EBPF {

class EBPFExprStepper : public ExprStepper {
 protected:
    std::string getClassName() override { return "EBPFExprStepper"; }

 public:
    EBPFExprStepper(ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo);

    void evalExternMethodCall(const IR::MethodCallExpression *call, const IR::Expression *receiver,
                              IR::ID name, const IR::Vector<IR::Argument> *args,
                              ExecutionState &state) override;

    bool preorder(const IR::P4Table * /*table*/) override;

    /// @returns a method call statement that calls packet_out.emit on the provided input label.
    static const IR::MethodCallStatement *produceEmitCall(const IR::Member *fieldLabel);
};
}  // namespace P4Tools::P4Testgen::EBPF

#endif /* TESTGEN_TARGETS_EBPF_EXPR_STEPPER_H_ */
