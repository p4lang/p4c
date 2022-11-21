#ifndef TESTGEN_TARGETS_EBPF_TARGET_H_
#define TESTGEN_TARGETS_EBPF_TARGET_H_

#include <stdint.h>

#include <boost/filesystem.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"

#include "backends/p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/core/target.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/targets/ebpf/cmd_stepper.h"
#include "backends/p4tools/testgen/targets/ebpf/expr_stepper.h"
#include "backends/p4tools/testgen/targets/ebpf/program_info.h"
#include "backends/p4tools/testgen/targets/ebpf/test_backend.h"
#include "p4tools/testgen/core/arch_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

class EBPFTestgenTarget : public TestgenTarget {
 public:
    /// Registers this target.
    static void make();

 protected:
    const EBPFProgramInfo* initProgram_impl(
        const IR::P4Program* program, const IR::Declaration_Instance* mainDecl) const override;

    int getPortNumWidth_bits_impl() const override;

    EBPFTestBackend* getTestBackend_impl(const ProgramInfo& programInfo,
                                         ExplorationStrategy& symbex,
                                         const boost::filesystem::path& testPath,
                                         boost::optional<uint32_t> seed) const override;

    EBPFCmdStepper* getCmdStepper_impl(ExecutionState& state, AbstractSolver& solver,
                                       const ProgramInfo& programInfo) const override;

    EBPFExprStepper* getExprStepper_impl(ExecutionState& state, AbstractSolver& solver,
                                         const ProgramInfo& programInfo) const override;

    const ArchSpec* getArchSpecImpl() const override;

 private:
    EBPFTestgenTarget();

    static const ArchSpec archSpec;
};

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_TARGET_H_ */
