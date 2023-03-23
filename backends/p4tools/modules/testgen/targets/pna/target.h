#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TARGET_H_

#include <cstdint>

#include <boost/filesystem.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/arch_spec.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/program_info.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_backend.h"

namespace P4Tools::P4Testgen::Pna {

class PnaDpdkTestgenTarget : public TestgenTarget {
 public:
    /// Registers this target.
    static void make();

 protected:
    const PnaDpdkProgramInfo *initProgram_impl(
        const IR::P4Program *program, const IR::Declaration_Instance *mainDecl) const override;

    int getPortNumWidth_bits_impl() const override;

    PnaTestBackend *getTestBackend_impl(const ProgramInfo &programInfo, SymbolicExecutor &symbex,
                                        const boost::filesystem::path &testPath,
                                        boost::optional<uint32_t> seed) const override;

    PnaDpdkCmdStepper *getCmdStepper_impl(ExecutionState &state, AbstractSolver &solver,
                                          const ProgramInfo &programInfo) const override;

    PnaDpdkExprStepper *getExprStepper_impl(ExecutionState &state, AbstractSolver &solver,
                                            const ProgramInfo &programInfo) const override;

    const ArchSpec *getArchSpecImpl() const override;

 private:
    PnaDpdkTestgenTarget();

    static const ArchSpec archSpec;
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TARGET_H_ */
