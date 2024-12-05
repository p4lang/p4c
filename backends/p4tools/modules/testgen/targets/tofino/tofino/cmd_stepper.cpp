/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include "backends/p4tools/modules/testgen/targets/tofino/tofino/cmd_stepper.h"

#include <cstddef>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include <boost/none.hpp>

#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/tofino/bf-p4c/ir/gress.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/targets/tofino/check_parser_error.h"
#include "backends/p4tools/modules/testgen/targets/tofino/constants.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino/program_info.h"

namespace P4::P4Tools::P4Testgen::Tofino {

const TofinoProgramInfo &TofinoCmdStepper::getProgramInfo() const {
    return *CmdStepper::getProgramInfo().to<TofinoProgramInfo>();
}

void TofinoCmdStepper::initializeTargetEnvironment(ExecutionState &nextState) const {
    auto programInfo = getProgramInfo();
    const auto &archSpec = programInfo.getArchSpec();
    const auto *pipes = programInfo.getPipes();

    auto initToTaint = [&](const IR::Member *member) {
        nextState.set(member, ToolsVariables::getTaintExpression(member->type));
    };

    // Initialize all the parameters with their default values.
    // TODO: Respect autoinit_metadata annotations.
    for (const auto &pipe : *pipes) {
        size_t blockIdx = 0;
        for (const auto &blockTuple : pipe.pipes) {
            const auto *typeDecl = blockTuple.second;
            const auto *archMember = archSpec.getArchMember(blockIdx);
            nextState.initializeBlockParams(TestgenTarget::get(), typeDecl,
                                            &archMember->blockParams);
            blockIdx++;
        }
    }

    // Set intrinsic metadata fields. Fields that are out of our control are tainted.
    const auto *oneBitType = IR::Type_Bits::get(1);
    const auto *nineBitType = IR::Type_Bits::get(1);
    const auto *sixteenBitType = IR::Type_Bits::get(1);
    const auto *queueIdType = IR::Type_Bits::get(5);

    // ingress_intrinsic_metadata_t
    const auto *igIntrMd = new IR::PathExpression(new IR::Path("*ig_intr_md"));
    // Set the Tofino metadata to valid for initialization. This might be undone in the parser.
    // This is because the parser parameter direction for metadata is "out".
    nextState.set(ToolsVariables::getHeaderValidity(igIntrMd), IR::BoolLiteral::get(true));
    // Flag distinguishing original packets from resubmitted packets.
    // Don't model resubmitted packets for now: set resubmit_flag to 0.
    nextState.set(new IR::Member(oneBitType, igIntrMd, "resubmit_flag"),
                  IR::Constant::get(oneBitType, 0));
    // Ingress IEEE 1588 timestamp (in nsec)
    initToTaint(new IR::Member(IR::Type_Bits::get(48), igIntrMd, "ingress_mac_tstamp"));

    // ingress_intrinsic_metadata_from_parser_t
    const auto *igIntrPrsMd = new IR::PathExpression(new IR::Path("*ig_intr_md_from_prsr"));
    // Global timestamp (ns) taken upon arrival at ingress.
    initToTaint(new IR::Member(IR::Type_Bits::get(48), igIntrPrsMd, "global_tstamp"));
    // Global version number taken upon arrival at ingress.
    initToTaint(new IR::Member(IR::Type_Bits::get(32), igIntrPrsMd, "global_ver"));

    // egress_intrinsic_metadata_t
    const auto *egIntrMd = new IR::PathExpression(new IR::Path("*eg_intr_md"));
    // Set the Tofino metadata to valid for initialization. This might be undone in the parser.
    // This is because the parser parameter direction for metadata is "out".
    nextState.set(ToolsVariables::getHeaderValidity(egIntrMd), IR::BoolLiteral::get(true));
    // Queue depth at the packet enqueue time.
    initToTaint(new IR::Member(IR::Type_Bits::get(19), egIntrMd, "enq_qdepth"));
    // Queue congestion status at the packet  enqueue time.
    initToTaint(new IR::Member(IR::Type_Bits::get(2), egIntrMd, "enq_congest_stat"));
    // Time snapshot taken when the packet is enqueued (in nsec).
    initToTaint(new IR::Member(IR::Type_Bits::get(18), egIntrMd, "enq_tstamp"));
    // Queue depth at the packet dequeue time.
    initToTaint(new IR::Member(IR::Type_Bits::get(19), egIntrMd, "dep_qdepth"));
    // Queue congestion status at the packet dequeue time.
    initToTaint(new IR::Member(IR::Type_Bits::get(2), egIntrMd, "dep_congest_stat"));
    // Dequeue-time application-pool congestion status. 2 bits per pool.
    initToTaint(new IR::Member(IR::Type_Bits::get(8), egIntrMd, "app_pool_congest_stat"));
    // Time delta between the packet's enqueue and dequeue time.
    initToTaint(new IR::Member(IR::Type_Bits::get(18), egIntrMd, "deq_timedelta"));
    // L3 replication id for multicast packets.
    initToTaint(new IR::Member(sixteenBitType, egIntrMd, "egress_rid"));
    // Flag indicating the first replica for the given multicast group.
    initToTaint(new IR::Member(oneBitType, egIntrMd, "egress_rid_first"));
    // Egress (physical) queue id via which this packet was served.
    initToTaint(new IR::Member(queueIdType, egIntrMd, "egress_qid"));
    // Egress cos (eCoS) value.
    initToTaint(new IR::Member(IR::Type_Bits::get(3), egIntrMd, "egress_cos"));
    // Flag indicating whether a packet is deflected due to deflect_on_drop.
    // For now, assume that the deflection flag in egress is always 0.
    nextState.set(new IR::Member(oneBitType, egIntrMd, "deflection_flag"),
                  IR::Constant::get(oneBitType, 0));
    // Packet length, in bytes.
    // TODO: Fix the packet size width. It should be 16 by default. Then we can calculate a packet.
    initToTaint(new IR::Member(IR::Type_Bits::get(16), egIntrMd, "pkt_length"));

    // ingress_intrinsic_metadata_from_parser_t
    const auto *egIntrPrsMd = new IR::PathExpression(new IR::Path("*eg_intr_md_from_prsr"));
    // Global timestamp (ns) taken upon arrival at egress.
    initToTaint(new IR::Member(IR::Type_Bits::get(48), egIntrPrsMd, "global_tstamp"));
    // Global version number taken upon arrival at egress.
    initToTaint(new IR::Member(IR::Type_Bits::get(32), egIntrPrsMd, "global_ver"));

    // Set the initial drop var to zero.
    nextState.set(&TofinoConstants::INGRESS_DROP_VAR,
                  IR::Constant::get(TofinoConstants::INGRESS_DROP_VAR.type, 0));
    nextState.set(&TofinoConstants::EGRESS_DROP_VAR,
                  IR::Constant::get(TofinoConstants::EGRESS_DROP_VAR.type, 0));

    // Initialize the input port.
    const auto &ingressPort = programInfo.getTargetInputPortVar();
    // TODO: If egress port is in loopback mode, we should restart Ingress and assign
    // egress port to ingress port here.
    nextState.set(ingressPort,
                  ToolsVariables::getSymbolicVariable(ingressPort->type, "tofino_ingress_port"_cs));
    // Initialize the global egress port variable. We initially set it to taint, which means drop.
    nextState.set(getProgramInfo().getTargetOutputPortVar(),
                  new IR::UninitializedTaintExpression(nineBitType));

    // Tofino appends a frame check sequence to the packet. Initialize it.
    nextState.setProperty("fcsLeft"_cs, TofinoConstants::ETH_FCS_SIZE);
}

std::optional<const Constraint *> TofinoCmdStepper::startParserImpl(
    const IR::P4Parser *parser, ExecutionState &nextState) const {
    auto programINfo = getProgramInfo();
    auto gress = programINfo.getGress(parser);

    const auto *nineBitType = IR::Type_Bits::get(9);
    const auto *oneBitType = IR::Type_Bits::get(1);
    const auto *parserErrorType = programInfo.getParserErrorType();

    // Check for pa_auto_init_metadata, which sets metadata values to 0.
    auto parserParams = parser->getApplyParameters()->parameters;
    for (std::size_t idx = 1; idx != parserParams.size(); ++idx) {
        const auto *param = parserParams.at(idx);
        const auto *paramType = param->type;
        if (const auto *typeName = paramType->to<IR::Type_Name>()) {
            paramType = nextState.resolveType(typeName);
        }
        const auto *localMdTsType = paramType->checkedTo<IR::Type_StructLike>();
        // Check whether the structure has a "pa_auto_init_metadata" annotation associated with it.
        const auto *initMeta = localMdTsType->getAnnotation("pa_auto_init_metadata"_cs);
        // If yes, set all values to 0.
        if (initMeta != nullptr) {
            const auto *paramRef = new IR::PathExpression(paramType, new IR::Path(param->name));
            declareStructLike(nextState, paramRef);
        }
    }

    switch (gress) {
        case INGRESS: {
            // bypass_egress is not active at the beginning.
            const auto &bypassExpr =
                programINfo.getParserParamVar(parser, oneBitType, 4, "bypass_egress"_cs);
            nextState.set(bypassExpr, IR::Constant::get(oneBitType, 0));

            // Check whether the parser error was referenced in this particular pipe.
            const auto *pipes = programINfo.getPipes();
            const auto *decl = pipes->at(programINfo.getPipeIdx(parser)).pipes.at("IngressT"_cs);
            const auto *p4control = decl->checkedTo<IR::P4Control>();
            CheckParserError checkParserError;
            p4control->apply(checkParserError);
            nextState.setProperty("parserErrReferenced"_cs, checkParserError.hasParserError());
            // Tofino implicitly drops packets if they are not set during the ingress or egress
            // pipeline.
            nextState.set(
                programINfo.getParserParamVar(parser, nineBitType, 4, "ucast_egress_port"_cs),
                new IR::UninitializedTaintExpression(nineBitType));
            // Initialize parser_err with no error and set the parser error label.
            // This label is used by some core externs to control target's parser error.
            const auto &parserErrorLabelExpr =
                programINfo.getParserParamVar(parser, parserErrorType, 5, "parser_err"_cs);
            nextState.set(parserErrorLabelExpr, IR::Constant::get(parserErrorType, 0));
            nextState.setParserErrorLabel(parserErrorLabelExpr);
            nextState.setProperty("gress"_cs, static_cast<uint64_t>(gress_t::INGRESS));
            break;
        }

        case EGRESS: {
            // Retrieve the egress port associated with this parser.
            // Tofino implicitly drops packets if they are not set during the ingress or egress
            // pipeline.
            nextState.set(programINfo.getParserParamVar(parser, nineBitType, 3, "egress_port"_cs),
                          new IR::UninitializedTaintExpression(nineBitType));
            // Initialize parser_err with no error and set the parser error label.
            // This label is used by some core externs to control target's parser error.
            const auto &parserErrorLabelExpr =
                programINfo.getParserParamVar(parser, parserErrorType, 3, "parser_err"_cs);
            nextState.set(parserErrorLabelExpr, IR::Constant::get(parserErrorType, 0));
            nextState.setParserErrorLabel(parserErrorLabelExpr);
            nextState.setProperty("gress"_cs, static_cast<uint64_t>(gress_t::EGRESS));
            break;
        }

        default:
            BUG("Unimplemented thread: %1%", gress);
    }

    return std::nullopt;
}

std::map<Continuation::Exception, Continuation> TofinoCmdStepper::getExceptionHandlers(
    const IR::P4Parser *parser, Continuation::Body /*normalContinuation*/,
    const ExecutionState &nextState) const {
    // According to TNA, the ingress parser drops the packet when any of the following is true:
    //   - parser_err is non-zero and the ingress MAU doesn't refer to parser_err
    //   - the PARSER_ERROR_NO_MATCH bit in parser_err is set
    //
    // The egress parser never drops the packet.
    const auto programInfo = getProgramInfo();
    const auto *parserErrorType = programInfo.getParserErrorType();

    std::map<Continuation::Exception, Continuation> result;
    auto gress = programInfo.getGress(parser);
    const auto *exitCall =
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("tofino_drop", {}));
    // Set the parser error label to PARSER_ERROR_NO_MATCH in case of a no-match.
    switch (gress) {
        case INGRESS: {
            // TODO: Implement the conditions above. Currently, this always drops the packet.
            // Suggest refactoring `result` so that its values are lists of (path condition,
            // continuation) pairs. This would allow us to branch on the value of parser_err.
            // If the ingress MAU references the parser error the parser does not drop the
            // packet.
            if (nextState.getProperty<bool>("parserErrReferenced"_cs)) {
                result.emplace(Continuation::Exception::PacketTooShort, Continuation::Body({}));
            } else {
                result.emplace(Continuation::Exception::PacketTooShort,
                               Continuation::Body({exitCall}));
            }

            // Models PARSER_ERROR_NO_TCAM parser error.
            // Set the parser error label to PARSER_ERROR_NO_MATCH in case of a no-match.
            const auto &parserErrorVariable =
                programInfo.getParserParamVar(parser, parserErrorType, 5, "parser_err"_cs);
            const auto *noMatchConst = IR::Constant::get(parserErrorVariable->type,
                                                         TofinoConstants::PARSER_ERROR_NO_MATCH);
            const auto *noMatchAssign = new IR::AssignmentStatement(
                parserErrorVariable,
                new IR::BOr(parserErrorVariable->type, parserErrorVariable, noMatchConst));
            result.emplace(Continuation::Exception::NoMatch,
                           Continuation::Body({noMatchAssign, exitCall}));
            result.emplace(Continuation::Exception::Reject, Continuation::Body({}));
            break;
        }

        case EGRESS: {
            result.emplace(Continuation::Exception::PacketTooShort, Continuation::Body({}));
            const auto &parserErrorVariable =
                programInfo.getParserParamVar(parser, parserErrorType, 3, "parser_err"_cs);
            const auto *noMatchConst = IR::Constant::get(parserErrorVariable->type,
                                                         TofinoConstants::PARSER_ERROR_NO_MATCH);
            const auto *noMatchAssign = new IR::AssignmentStatement(
                parserErrorVariable,
                new IR::BOr(parserErrorVariable->type, parserErrorVariable, noMatchConst));
            result.emplace(Continuation::Exception::NoMatch, Continuation::Body({noMatchAssign}));
            result.emplace(Continuation::Exception::Reject, Continuation::Body({}));
            break;
        }

        case GHOST:
        default:
            BUG("Unimplemented thread: %1%", gress);
    }

    return result;
}

TofinoCmdStepper::TofinoCmdStepper(ExecutionState &state, AbstractSolver &solver,
                                   const ProgramInfo &programInfo)
    : CmdStepper(state, solver, programInfo) {}

}  // namespace P4::P4Tools::P4Testgen::Tofino
