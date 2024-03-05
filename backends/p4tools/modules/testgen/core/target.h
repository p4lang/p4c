#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_TARGET_H_

#include <filesystem>
#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "ir/ir.h"
#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"

namespace P4Tools::P4Testgen {

class TestgenTarget : public Target {
 public:
    /// @returns the singleton instance for the current target.
    static const TestgenTarget &get();

    /// Produces a @ProgramInfo for the given P4 program.
    ///
    /// @returns nullptr if the program is not supported by this target.
    static const ProgramInfo *produceProgramInfo(const CompilerResult &compilerResult);

    /// Returns the test back end associated with this P4Testgen target.
    static TestBackEnd *getTestBackend(const ProgramInfo &programInfo,
                                       const TestBackendConfiguration &testBackendConfiguration,
                                       SymbolicExecutor &symbex);

    /// Provides a CmdStepper implementation for this target.
    static CmdStepper *getCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                     const ProgramInfo &programInfo);

    /// Provides a ExprStepper implementation for this target.
    static ExprStepper *getExprStepper(ExecutionState &state, AbstractSolver &solver,
                                       const ProgramInfo &programInfo);

 protected:
    /// @see @produceProgramInfo.
    [[nodiscard]] const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult) const;

    /// @see @produceProgramInfo.
    virtual const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const = 0;

    /// @see getTestBackend.
    virtual TestBackEnd *getTestBackendImpl(
        const ProgramInfo &programInfo, const TestBackendConfiguration &testBackendConfiguration,
        SymbolicExecutor &symbex) const = 0;

    /// @see getCmdStepper.
    virtual CmdStepper *getCmdStepperImpl(ExecutionState &state, AbstractSolver &solver,
                                          const ProgramInfo &programInfo) const = 0;

    /// @see getExprStepper.
    virtual ExprStepper *getExprStepperImpl(ExecutionState &state, AbstractSolver &solver,
                                            const ProgramInfo &programInfo) const = 0;

    explicit TestgenTarget(std::string deviceName, std::string archName);
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_TARGET_H_ */
