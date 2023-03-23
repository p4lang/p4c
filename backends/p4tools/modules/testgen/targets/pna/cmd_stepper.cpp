#include "backends/p4tools/modules/testgen/targets/pna/cmd_stepper.h"

#include <cstddef>
#include <list>
#include <map>
#include <utility>

#include <boost/none.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/arch_spec.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"
#include "backends/p4tools/modules/testgen/targets/pna/program_info.h"

namespace P4Tools::P4Testgen::Pna {

PnaDpdkCmdStepper::PnaDpdkCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                     const ProgramInfo &programInfo)
    : CmdStepper(state, solver, programInfo) {}

const PnaDpdkProgramInfo &PnaDpdkCmdStepper::getProgramInfo() const {
    return *CmdStepper::getProgramInfo().checkedTo<PnaDpdkProgramInfo>();
}

void PnaDpdkCmdStepper::initializeTargetEnvironment(ExecutionState *nextState) const {
    const auto &programInfo = getProgramInfo();
    const auto *archSpec = TestgenTarget::getArchSpec();
    const auto *programmableBlocks = programInfo.getProgrammableBlocks();

    // PNA initializes all metadata to zero. To avoid unnecessary taint, we retrieve the type and
    // initialize all the relevant metadata variables to zero.
    size_t blockIdx = 0;
    for (const auto &blockTuple : *programmableBlocks) {
        const auto *typeDecl = blockTuple.second;
        const auto *archMember = archSpec->getArchMember(blockIdx);
        initializeBlockParams(typeDecl, &archMember->blockParams, nextState);
        blockIdx++;
    }
    const auto *thirtytwoBitType = IR::getBitType(32);
    nextState->set(&PnaConstants::DROP_VAR, IR::getBoolLiteral(false));
    // PNA implicitly sets the output port to 0.
    nextState->set(&PnaConstants::OUTPUT_PORT_VAR, IR::getConstant(thirtytwoBitType, 0));
    // PNA implicitly sets the output port to 0.
    nextState->set(new IR::Member(thirtytwoBitType, new IR::PathExpression("*istd"), "direction"),
                   nextState->createZombieConst(thirtytwoBitType, "*pna_direction"));
}

boost::optional<const Constraint *> PnaDpdkCmdStepper::startParser_impl(
    const IR::P4Parser * /*parser*/, ExecutionState *nextState) const {
    // We need to explicitly map the parser error
    const auto *parserErrVar = new IR::Member(
        IR::getBitType(32), getProgramInfo().getBlockParam("PreControlT", 3), "parser_error");
    nextState->setParserErrorLabel(parserErrVar);
    return boost::none;
}

std::map<Continuation::Exception, Continuation> PnaDpdkCmdStepper::getExceptionHandlers(
    const IR::P4Parser * /*parser*/, Continuation::Body /*normalContinuation*/,
    const ExecutionState * /*nextState*/) const {
    std::map<Continuation::Exception, Continuation> result;
    const auto *parserErrVar = new IR::Member(
        IR::getBitType(32), getProgramInfo().getBlockParam("PreControlT", 3), "parser_error");

    result.emplace(Continuation::Exception::Reject, Continuation::Body({}));
    result.emplace(Continuation::Exception::PacketTooShort,
                   Continuation::Body({new IR::AssignmentStatement(
                       parserErrVar, IR::getConstant(parserErrVar->type, 1))}));
    // NoMatch will transition to the next block.
    result.emplace(Continuation::Exception::NoMatch, Continuation::Body({}));

    return result;
}

}  // namespace P4Tools::P4Testgen::Pna
