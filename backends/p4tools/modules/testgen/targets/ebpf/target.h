#ifndef TESTGEN_TARGETS_EBPF_TARGET_H_
#define TESTGEN_TARGETS_EBPF_TARGET_H_

#include <cstdint>
#include <filesystem>
#include <optional>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/arch_spec.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/program_info.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/test_backend.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

class EBPFTestgenTarget : public TestgenTarget {
 public:
    /// Registers this target.
    static void make();

 protected:
    const EBPFProgramInfo *initProgram_impl(
        const IR::P4Program *program, const IR::Declaration_Instance *mainDecl) const override;

    int getPortNumWidth_bits_impl() const override;

    EBPFTestBackend *getTestBackend_impl(const ProgramInfo &programInfo, SymbolicExecutor &symbex,
                                         const std::filesystem::path &testPath,
                                         std::optional<uint32_t> seed) const override;

    EBPFCmdStepper *getCmdStepper_impl(ExecutionState &state, AbstractSolver &solver,
                                       const ProgramInfo &programInfo) const override;

    EBPFExprStepper *getExprStepper_impl(ExecutionState &state, AbstractSolver &solver,
                                         const ProgramInfo &programInfo) const override;

    const ArchSpec *getArchSpecImpl() const override;

 private:
    EBPFTestgenTarget();

    static const ArchSpec archSpec;
};

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_TARGET_H_ */
