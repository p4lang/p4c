#ifndef TESTGEN_TARGETS_EBPF_CMD_STEPPER_H_
#define TESTGEN_TARGETS_EBPF_CMD_STEPPER_H_

#include <map>
#include <optional>
#include <string>

#include "ir/ir.h"
#include "ir/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/program_info.h"

namespace P4Tools::P4Testgen::EBPF {

class EBPFCmdStepper : public CmdStepper {
 protected:
    std::string getClassName() override { return "EBPFCmdStepper"; }

    const EBPFProgramInfo &getProgramInfo() const override;

    void initializeTargetEnvironment(ExecutionState &nextState) const override;

    std::optional<const Constraint *> startParserImpl(const IR::P4Parser *parser,
                                                      ExecutionState &nextState) const override;

    std::map<Continuation::Exception, Continuation> getExceptionHandlers(
        const IR::P4Parser *parser, Continuation::Body normalContinuation,
        const ExecutionState &nextState) const override;

 public:
    EBPFCmdStepper(ExecutionState &nextState, AbstractSolver &solver,
                   const ProgramInfo &programInfo);
};

}  // namespace P4Tools::P4Testgen::EBPF

#endif /* TESTGEN_TARGETS_EBPF_CMD_STEPPER_H_ */
