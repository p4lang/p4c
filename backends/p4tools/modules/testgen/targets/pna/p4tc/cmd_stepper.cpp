#include "backends/p4tools/modules/testgen/targets/pna/p4tc/cmd_stepper.h"

#include <cstddef>
#include <list>
#include <map>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
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
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"
#include "backends/p4tools/modules/testgen/targets/pna/p4tc/program_info.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_cmd_stepper.h"

namespace P4::P4Tools::P4Testgen::Pna {

std::string PnaP4TCCmdStepper::getClassName() { return "PnaP4TCCmdStepper"; }

PnaP4TCCmdStepper::PnaP4TCCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                     const ProgramInfo &programInfo)
    : SharedPnaCmdStepper(state, solver, programInfo) {}

const PnaP4TCProgramInfo &PnaP4TCCmdStepper::getProgramInfo() const {
    return *CmdStepper::getProgramInfo().checkedTo<PnaP4TCProgramInfo>();
}

void PnaP4TCCmdStepper::initializeTargetEnvironment(ExecutionState &nextState) const {
    const auto &programInfo = getProgramInfo();
    const auto &target = TestgenTarget::get();
    const auto *programmableBlocks = programInfo.getProgrammableBlocks();

    // PNA initializes all metadata to zero. To avoid unnecessary taint, we retrieve the type and
    // initialize all the relevant metadata variables to zero.
    size_t blockIdx = 0;
    for (const auto &blockTuple : *programmableBlocks) {
        const auto *typeDecl = blockTuple.second;
        const auto *archMember = programInfo.getArchSpec().getArchMember(blockIdx);
        nextState.initializeBlockParams(target, typeDecl, &archMember->blockParams);
        blockIdx++;
    }
    const auto *thirtytwoBitType = IR::Type_Bits::get(32);
    nextState.set(&PnaConstants::DROP_VAR, IR::BoolLiteral::get(false));
    // PNA implicitly sets the output port to 0.
    nextState.set(&PnaConstants::OUTPUT_PORT_VAR, IR::Constant::get(thirtytwoBitType, 0));
    // Initialize the direction metadata variables.
}

std::optional<const Constraint *> PnaP4TCCmdStepper::startParserImpl(
    const IR::P4Parser * /*parser*/, ExecutionState &nextState) const {
    // We need to explicitly map the parser error
    nextState.setParserErrorLabel(&PnaConstants::PARSER_ERROR);
    // Initialize the parser error to 0.
    nextState.set(&PnaConstants::PARSER_ERROR,
                  IR::Constant::get(programInfo.getParserErrorType(), 0));
    return std::nullopt;
}

std::map<Continuation::Exception, Continuation> PnaP4TCCmdStepper::getExceptionHandlers(
    const IR::P4Parser * /*parser*/, Continuation::Body /*normalContinuation*/,
    const ExecutionState & /*nextState*/) const {
    std::map<Continuation::Exception, Continuation> result;
    auto programInfo = getProgramInfo();

    auto *exitCall =
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("drop_and_exit", {}));
    result.emplace(Continuation::Exception::Reject, Continuation::Body({exitCall}));
    result.emplace(Continuation::Exception::PacketTooShort, Continuation::Body({exitCall}));
    // NoMatch is equivalent to Reject for p4tc.
    result.emplace(Continuation::Exception::NoMatch, Continuation::Body({exitCall}));

    return result;
}

}  // namespace P4::P4Tools::P4Testgen::Pna
