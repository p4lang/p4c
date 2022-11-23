#ifndef TESTGEN_TARGETS_EBPF_CMD_STEPPER_H_
#define TESTGEN_TARGETS_EBPF_CMD_STEPPER_H_

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
#include "backends/p4tools/modules/testgen/targets/ebpf/program_info.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

class EBPFCmdStepper : public CmdStepper {
 protected:
    std::string getClassName() override { return "EBPFCmdStepper"; }

    const EBPFProgramInfo& getProgramInfo() const override;

    void initializeTargetEnvironment(ExecutionState* nextState) const override;

    boost::optional<const Constraint*> startParser_impl(const IR::P4Parser* parser,
                                                        ExecutionState* state) const override;

    std::map<Continuation::Exception, Continuation> getExceptionHandlers(
        const IR::P4Parser* parser, Continuation::Body normalContinuation,
        const ExecutionState* state) const override;

 public:
    EBPFCmdStepper(ExecutionState& state, AbstractSolver& solver, const ProgramInfo& programInfo);
};

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_CMD_STEPPER_H_ */
