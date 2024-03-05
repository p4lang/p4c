#include "backends/p4tools/modules/testgen/targets/ebpf/cmd_stepper.h"

#include <cstddef>
#include <list>
#include <map>
#include <optional>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/program_info.h"

namespace P4Tools::P4Testgen::EBPF {

EBPFCmdStepper::EBPFCmdStepper(ExecutionState &state, AbstractSolver &solver,
                               const ProgramInfo &programInfo)
    : CmdStepper(state, solver, programInfo) {}

const EBPFProgramInfo &EBPFCmdStepper::getProgramInfo() const {
    return *CmdStepper::getProgramInfo().to<EBPFProgramInfo>();
}

void EBPFCmdStepper::initializeTargetEnvironment(ExecutionState &nextState) const {
    const auto &programInfo = getProgramInfo();
    const auto &target = TestgenTarget::get();
    const auto *programmableBlocks = programInfo.getProgrammableBlocks();

    // eBPF initializes all metadata to zero. To avoid unnecessary taint, we retrieve the type and
    // initialize all the relevant metadata variables to zero.
    size_t blockIdx = 0;
    for (const auto &blockTuple : *programmableBlocks) {
        const auto *typeDecl = blockTuple.second;
        const auto *archMember = programInfo.getArchSpec().getArchMember(blockIdx);
        nextState.initializeBlockParams(target, typeDecl, &archMember->blockParams);
        blockIdx++;
    }

    const auto *nineBitType = IR::getBitType(9);
    // Set the input ingress port to 0.
    nextState.set(programInfo.getTargetInputPortVar(), IR::getConstant(nineBitType, 0));
    // eBPF implicitly sets the output port to 0. In reality, there is no output port.
    nextState.set(programInfo.getTargetOutputPortVar(), IR::getConstant(nineBitType, 0));
    // We need to explicitly set the parser error. There is no eBPF metadata.
    const auto *errVar = new IR::Member(new IR::PathExpression("*"), "parser_err");
    nextState.setParserErrorLabel(errVar);
}

std::optional<const Constraint *> EBPFCmdStepper::startParserImpl(
    const IR::P4Parser * /*parser*/, ExecutionState & /*nextState*/) const {
    return std::nullopt;
}

std::map<Continuation::Exception, Continuation> EBPFCmdStepper::getExceptionHandlers(
    const IR::P4Parser * /*parser*/, Continuation::Body /*normalContinuation*/,
    const ExecutionState & /*nextState*/) const {
    std::map<Continuation::Exception, Continuation> result;
    auto programInfo = getProgramInfo();

    auto *exitCall =
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("drop_and_exit", {}));
    result.emplace(Continuation::Exception::Reject, Continuation::Body({exitCall}));
    result.emplace(Continuation::Exception::PacketTooShort, Continuation::Body({exitCall}));
    // NoMatch is equivalent to Reject in ebpf_model.
    result.emplace(Continuation::Exception::NoMatch, Continuation::Body({exitCall}));

    return result;
}

}  // namespace P4Tools::P4Testgen::EBPF
