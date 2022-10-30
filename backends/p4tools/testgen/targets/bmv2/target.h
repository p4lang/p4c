#ifndef BACKENDS_P4TOOLS_TESTGEN_TARGETS_BMV2_TARGET_H_
#define BACKENDS_P4TOOLS_TESTGEN_TARGETS_BMV2_TARGET_H_

#include <stdint.h>

#include <boost/filesystem.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"

#include "backends/p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/core/target.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/targets/bmv2/cmd_stepper.h"
#include "backends/p4tools/testgen/targets/bmv2/expr_stepper.h"
#include "backends/p4tools/testgen/targets/bmv2/program_info.h"
#include "backends/p4tools/testgen/targets/bmv2/test_backend.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

class BMv2_V1ModelTestgenTarget : public TestgenTarget {
 public:
    /// Registers this target.
    static void make();

 protected:
    const BMv2_V1ModelProgramInfo* initProgram_impl(
        const IR::P4Program* program, const IR::Declaration_Instance* mainDecl) const override;

    int getPortNumWidth_bits_impl() const override;

    Bmv2TestBackend* getTestBackend_impl(const ProgramInfo& programInfo,
                                         ExplorationStrategy& symbex,
                                         const boost::filesystem::path& testPath,
                                         boost::optional<uint32_t> seed) const override;

    BMv2_V1ModelCmdStepper* getCmdStepper_impl(ExecutionState& state, AbstractSolver& solver,
                                               const ProgramInfo& programInfo) const override;

    BMv2_V1ModelExprStepper* getExprStepper_impl(ExecutionState& state, AbstractSolver& solver,
                                                 const ProgramInfo& programInfo) const override;

    const ArchSpec* getArchSpecImpl() const override;

 private:
    BMv2_V1ModelTestgenTarget();

    static const ArchSpec archSpec;
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_TARGETS_BMV2_TARGET_H_ */
