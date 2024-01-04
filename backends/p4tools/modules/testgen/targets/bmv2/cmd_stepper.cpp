#include "backends/p4tools/modules/testgen/targets/bmv2/cmd_stepper.h"

#include <cstddef>
#include <list>
#include <map>
#include <optional>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/constants.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"

namespace P4Tools::P4Testgen::Bmv2 {

Bmv2V1ModelCmdStepper::Bmv2V1ModelCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                             const ProgramInfo &programInfo)
    : CmdStepper(state, solver, programInfo) {}

const Bmv2V1ModelProgramInfo &Bmv2V1ModelCmdStepper::getProgramInfo() const {
    return *CmdStepper::getProgramInfo().checkedTo<Bmv2V1ModelProgramInfo>();
}

void Bmv2V1ModelCmdStepper::initializeTargetEnvironment(ExecutionState &nextState) const {
    // Associate intrinsic metadata with the packet. The input packet is prefixed with
    // ingress intrinsic metadata and port metadata.
    //
    // Ingress intrinsic metadata format from v1model.p4:
    // PortId_t    ingress_port;
    // PortId_t    egress_spec;
    // PortId_t    egress_port;
    // bit<32>     instance_type;
    // bit<32>     packet_length;
    // bit<32> enq_timestamp;
    // bit<19> enq_qdepth;
    // bit<32> deq_timedelta;
    // bit<19> deq_qdepth;

    // bit<48> ingress_global_timestamp;
    // bit<48> egress_global_timestamp;
    // bit<16> mcast_grp;
    // bit<16> egress_rid;
    // bit<1>  checksum_error;
    // error parser_error;
    // bit<3> priority;

    const auto &programInfo = getProgramInfo();
    const auto &target = TestgenTarget::get();
    const auto *programmableBlocks = programInfo.getProgrammableBlocks();

    // BMv2 initializes all metadata to zero. To avoid unnecessary taint, we retrieve the type and
    // initialize all the relevant metadata variables to zero.
    size_t blockIdx = 0;
    for (const auto &blockTuple : *programmableBlocks) {
        const auto *typeDecl = blockTuple.second;
        const auto *archMember = programInfo.getArchSpec().getArchMember(blockIdx);
        nextState.initializeBlockParams(target, typeDecl, &archMember->blockParams);
        blockIdx++;
    }

    const auto *nineBitType = IR::getBitType(9);
    const auto *oneBitType = IR::getBitType(1);
    nextState.set(programInfo.getTargetInputPortVar(),
                  ToolsVariables::getSymbolicVariable(nineBitType, "bmv2_ingress_port"));
    // BMv2 implicitly sets the output port to 0.
    nextState.set(programInfo.getTargetOutputPortVar(), IR::getConstant(nineBitType, 0));
    // Initialize parser_err with no error.
    const auto *parserErrVar =
        new IR::Member(programInfo.getParserErrorType(),
                       new IR::PathExpression("*standard_metadata"), "parser_error");
    nextState.set(parserErrVar, IR::getConstant(parserErrVar->type, 0));
    // Initialize checksum_error with no error.
    const auto *checksumErrVar =
        new IR::Member(oneBitType, new IR::PathExpression("*standard_metadata"), "checksum_error");
    nextState.set(checksumErrVar, IR::getConstant(checksumErrVar->type, 0));
    // The packet size meta data is the testgen packet length variable divided by 8.
    const auto *pktSizeType = &PacketVars::PACKET_SIZE_VAR_TYPE;
    const auto *packetSizeVar =
        new IR::Member(pktSizeType, new IR::PathExpression("*standard_metadata"), "packet_length");
    const auto *packetSizeConst = new IR::Div(pktSizeType, ExecutionState::getInputPacketSizeVar(),
                                              IR::getConstant(pktSizeType, 8));
    nextState.set(packetSizeVar, packetSizeConst);
}

std::optional<const Constraint *> Bmv2V1ModelCmdStepper::startParserImpl(
    const IR::P4Parser *parser, ExecutionState &nextState) const {
    // We need to explicitly map the parser error
    const auto *errVar = Bmv2V1ModelProgramInfo::getParserParamVar(
        parser, programInfo.getParserErrorType(), 3, "parser_error");
    nextState.setParserErrorLabel(errVar);

    return std::nullopt;
}

std::map<Continuation::Exception, Continuation> Bmv2V1ModelCmdStepper::getExceptionHandlers(
    const IR::P4Parser *parser, Continuation::Body /*normalContinuation*/,
    const ExecutionState & /*nextState*/) const {
    std::map<Continuation::Exception, Continuation> result;
    auto programInfo = getProgramInfo();
    auto gress = programInfo.getGress(parser);

    const auto *errVar = Bmv2V1ModelProgramInfo::getParserParamVar(
        parser, programInfo.getParserErrorType(), 3, "parser_error");

    switch (gress) {
        case BMV2_INGRESS:
            // TODO: Implement the conditions above. Currently, this always drops the packet.
            // Suggest refactoring `result` so that its values are lists of (path condition,
            // continuation) pairs. This would allow us to branch on the value of parser_err.
            // Would also need to augment ProgramInfo to detect whether the ingress MAU refers
            // to parser_err.

            ::warning("Ingress parser exception handler not fully implemented");
            result.emplace(Continuation::Exception::Reject, Continuation::Body({}));
            result.emplace(
                Continuation::Exception::PacketTooShort,
                Continuation::Body({new IR::AssignmentStatement(
                    errVar,
                    IR::getConstant(errVar->type, P4Constants::PARSER_ERROR_PACKET_TOO_SHORT))}));
            // NoMatch will transition to the next block.
            result.emplace(Continuation::Exception::NoMatch, Continuation::Body({}));
            break;

        // The egress parser never drops the packet.
        case BMV2_EGRESS:
            // NoMatch will transition to the next block.
            result.emplace(Continuation::Exception::NoMatch, Continuation::Body({}));
            break;
        default:
            BUG("Unimplemented thread: %1%", gress);
    }

    return result;
}

}  // namespace P4Tools::P4Testgen::Bmv2
