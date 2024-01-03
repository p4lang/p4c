#include "backends/p4tools/modules/testgen/targets/pna/dpdk/cmd_stepper.h"

#include <cstddef>
#include <list>
#include <map>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/arch_spec.h"
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
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/program_info.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_cmd_stepper.h"

namespace P4Tools::P4Testgen::Pna {

std::string PnaDpdkCmdStepper::getClassName() { return "PnaDpdkCmdStepper"; }

PnaDpdkCmdStepper::PnaDpdkCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                     const ProgramInfo &programInfo)
    : SharedPnaCmdStepper(state, solver, programInfo) {}

const PnaDpdkProgramInfo &PnaDpdkCmdStepper::getProgramInfo() const {
    return *CmdStepper::getProgramInfo().checkedTo<PnaDpdkProgramInfo>();
}

void PnaDpdkCmdStepper::initializeTargetEnvironment(ExecutionState &nextState) const {
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
    const auto *thirtytwoBitType = IR::getBitType(32);
    nextState.set(&PnaConstants::DROP_VAR, IR::getBoolLiteral(false));
    // PNA implicitly sets the output port to 0.
    nextState.set(&PnaConstants::OUTPUT_PORT_VAR, IR::getConstant(thirtytwoBitType, 0));
    // Initialize the direction metadata variables.
    nextState.set(
        new IR::Member(thirtytwoBitType, new IR::PathExpression("*pre_istd"), "direction"),
        &PnaSymbolicVars::DIRECTION);
    nextState.set(
        new IR::Member(thirtytwoBitType, new IR::PathExpression("*parser_istd"), "direction"),
        &PnaSymbolicVars::DIRECTION);
    nextState.set(
        new IR::Member(thirtytwoBitType, new IR::PathExpression("*main_istd"), "direction"),
        &PnaSymbolicVars::DIRECTION);
}

std::optional<const Constraint *> PnaDpdkCmdStepper::startParserImpl(
    const IR::P4Parser * /*parser*/, ExecutionState &nextState) const {
    // We need to explicitly map the parser error
    nextState.setParserErrorLabel(&PnaConstants::PARSER_ERROR);
    // Initialize the parser error to 0.
    nextState.set(&PnaConstants::PARSER_ERROR,
                  IR::getConstant(programInfo.getParserErrorType(), 0));
    return std::nullopt;
}

std::map<Continuation::Exception, Continuation> PnaDpdkCmdStepper::getExceptionHandlers(
    const IR::P4Parser * /*parser*/, Continuation::Body /*normalContinuation*/,
    const ExecutionState & /*nextState*/) const {
    std::map<Continuation::Exception, Continuation> result;
    result.emplace(Continuation::Exception::Reject, Continuation::Body({}));
    result.emplace(
        Continuation::Exception::PacketTooShort,
        Continuation::Body({new IR::AssignmentStatement(
            &PnaConstants::PARSER_ERROR, IR::getConstant(programInfo.getParserErrorType(), 1))}));
    // NoMatch will transition to the next block.
    result.emplace(Continuation::Exception::NoMatch, Continuation::Body({}));

    return result;
}

}  // namespace P4Tools::P4Testgen::Pna
