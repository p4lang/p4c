#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_CMD_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_CMD_STEPPER_H_

#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen::Pna {

class SharedPnaCmdStepper : public CmdStepper {
 public:
    SharedPnaCmdStepper(ExecutionState &state, AbstractSolver &solver,
                        const ProgramInfo &programInfo);
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_CMD_STEPPER_H_ */
