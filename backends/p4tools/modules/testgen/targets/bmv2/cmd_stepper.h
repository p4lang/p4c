#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CMD_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CMD_STEPPER_H_

#include <map>
#include <string>

#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

class BMv2_V1ModelCmdStepper : public CmdStepper {
 protected:
    std::string getClassName() override { return "BMv2_V1ModelCmdStepper"; }

    const BMv2_V1ModelProgramInfo &getProgramInfo() const override;

    void initializeTargetEnvironment(ExecutionState *nextState) const override;

    boost::optional<const Constraint *> startParser_impl(const IR::P4Parser *parser,
                                                         ExecutionState *state) const override;

    std::map<Continuation::Exception, Continuation> getExceptionHandlers(
        const IR::P4Parser *parser, Continuation::Body normalContinuation,
        const ExecutionState *state) const override;

 public:
    BMv2_V1ModelCmdStepper(ExecutionState &state, AbstractSolver &solver,
                           const ProgramInfo &programInfo);
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CMD_STEPPER_H_ */
