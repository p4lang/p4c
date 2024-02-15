#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TARGET_H_

#include <cstdint>
#include <filesystem>
#include <optional>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "ir/ir.h"
#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend.h"

namespace P4Tools::P4Testgen::Bmv2 {

class Bmv2V1ModelTestgenTarget : public TestgenTarget {
 public:
    /// Registers this target.
    static void make();

 protected:
    const Bmv2V1ModelProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult,
        const IR::Declaration_Instance *mainDecl) const override;

    Bmv2TestBackend *getTestBackendImpl(const ProgramInfo &programInfo,
                                        const TestBackendConfiguration &testBackendConfiguration,
                                        SymbolicExecutor &symbex) const override;

    Bmv2V1ModelCmdStepper *getCmdStepperImpl(ExecutionState &state, AbstractSolver &solver,
                                             const ProgramInfo &programInfo) const override;

    Bmv2V1ModelExprStepper *getExprStepperImpl(ExecutionState &state, AbstractSolver &solver,
                                               const ProgramInfo &programInfo) const override;

 private:
    Bmv2V1ModelTestgenTarget();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TARGET_H_ */
