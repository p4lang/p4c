#ifndef TESTGEN_TARGETS_EBPF_TARGET_H_
#define TESTGEN_TARGETS_EBPF_TARGET_H_

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
#include "backends/p4tools/modules/testgen/targets/ebpf/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/program_info.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/test_backend.h"

namespace P4Tools::P4Testgen::EBPF {

class EBPFTestgenTarget : public TestgenTarget {
 public:
    /// Registers this target.
    static void make();

 protected:
    const EBPFProgramInfo *initProgramImpl(const IR::P4Program *program,
                                           const IR::Declaration_Instance *mainDecl) const override;

    EBPFTestBackend *getTestBackendImpl(const ProgramInfo &programInfo, SymbolicExecutor &symbex,
                                        const std::filesystem::path &testPath) const override;

    EBPFCmdStepper *getCmdStepperImpl(ExecutionState &state, AbstractSolver &solver,
                                      const ProgramInfo &programInfo) const override;

    EBPFExprStepper *getExprStepperImpl(ExecutionState &state, AbstractSolver &solver,
                                        const ProgramInfo &programInfo) const override;

    const ArchSpec *getArchSpecImpl() const override;

 private:
    EBPFTestgenTarget();

    static const ArchSpec archSpec;
};

}  // namespace P4Tools::P4Testgen::EBPF

#endif /* TESTGEN_TARGETS_EBPF_TARGET_H_ */
