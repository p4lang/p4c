#include "backends/p4tools/modules/testgen/targets/pna/shared_cmd_stepper.h"

#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen::Pna {

SharedPnaCmdStepper::SharedPnaCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                         const ProgramInfo &programInfo)
    : CmdStepper(state, solver, programInfo) {}

}  // namespace P4Tools::P4Testgen::Pna
