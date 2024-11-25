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

#include "backends/p4tools/modules/testgen/targets/tofino/tofino/program_info.h"

#include <algorithm>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "ir/ir-generated.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/map.h"

#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/targets/tofino/concolic.h"
#include "backends/p4tools/modules/testgen/targets/tofino/constants.h"

namespace P4::P4Tools::P4Testgen::Tofino {

std::vector<std::vector<Continuation::Command>> TofinoProgramInfo::ingressCmds() const {
    return pipelineCmds(INGRESS);
}

std::vector<std::vector<Continuation::Command>> TofinoProgramInfo::egressCmds() const {
    return pipelineCmds(EGRESS);
}

std::optional<const IR::Expression *> TofinoProgramInfo::getPipePortRangeConstraint(
    const IR::StateVariable &portVar, size_t pipeIdx) const {
    // Pipe port ranges are defined Tofino Switch Architecture 3.1:
    //  Port[8:7] = Pipe number 0,1,2 or 3
    //  Port[6:0] = Port number withing a pipe
    const auto *pipeNumber = new IR::Slice(portVar, 8, 7);
    if (pipes.size() == 2) {
        // If we have 2 pipes, pipe ids 0 and 2 are connected to pipe1, 1 and 3 to pipe2.
        // The lower bit determines the pipe.
        size_t pipeScope = pipeIdx;
        return new IR::LOr(
            new IR::Equ(pipeNumber, new IR::Constant(pipeNumber->type, pipeScope)),
            new IR::Equ(pipeNumber, new IR::Constant(pipeNumber->type, pipeScope | 0x2)));
    }
    if (pipes.size() > 2) {
        return new IR::Equ(pipeNumber, new IR::Constant(pipeNumber->type, pipeIdx));
    }
    return {};
}

const IR::Expression *TofinoProgramInfo::getValidPortConstraint(
    const IR::StateVariable &portVar) const {
    const auto &options = TestgenOptions::get();
    // const auto *portNumber = new IR::Slice(portVar, 6, 0);

    // // 0x40-0x47 are recirc ports, 0x48 mirror, 0x49-0x4b reserved
    // auto *validMacPort =
    //     new IR::LAnd(new IR::Lss(portNumber, new IR::Constant(portNumber->type, 0x40)),
    //                  new IR::Leq(new IR::Constant(portNumber->type, 0), portNumber));

    // auto *allowedPort =
    //     new IR::Equ(new IR::BAnd(new IR::Neg(new IR::Constant(
    //                                  portVar->type,
    //                                  SharedTofinoConstants::VALID_INPUT_PORT_MASK)),
    //                              portVar),
    //                 new IR::Constant(portVar->type, 0));

    // const auto *validPortsCond = new IR::LAnd(validMacPort, allowedPort);
    const IR::Expression *validPortsCond = new IR::BoolLiteral(true);

    /// If the the vector of permitted port ranges is not empty, set the restrictions on the
    /// possible output port.
    if (!options.permittedPortRanges.empty()) {
        /// The drop port is also always a valid port. Initialize the condition with it.
        const IR::Expression *cond = new IR::BoolLiteral(false);
        for (const auto &portRange : options.permittedPortRanges) {
            const auto *loVarOut = IR::Constant::get(portVar->type, portRange.first);
            const auto *hiVarOut = IR::Constant::get(portVar->type, portRange.second);
            cond = new IR::LOr(
                cond, new IR::LAnd(new IR::Leq(loVarOut, portVar), new IR::Leq(portVar, hiVarOut)));
        }
        validPortsCond = new IR::LAnd(validPortsCond, cond);
    }
    return validPortsCond;
}

std::vector<std::vector<Continuation::Command>> TofinoProgramInfo::pipelineCmds(
    gress_t gress) const {
    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations.
    /// This sequence also includes nodes that handle transitions between the
    /// individual component instantiations.
    std::vector<std::vector<Continuation::Command>> pipelineSequences;
    const auto &archSpec = getArchSpec();

    for (const auto &pipeInfo : pipes) {
        pipelineSequences.emplace_back();
        auto &pipelineSequence = pipelineSequences.back();
        size_t blockIdx = 0;

        // Keep track of the pipe name throughout execution.
        pipelineSequence.emplace_back(
            Continuation::PropertyUpdate("pipe_name"_cs, pipeInfo.pipeName));

        for (const auto &decl : Values(pipeInfo.pipes)) {
            // Iterate through the (ordered) pipes of the target architecture.
            auto subCmds = processDeclaration(decl, blockIdx);
            if (getGress(decl) == gress) {
                pipelineSequence.insert(pipelineSequence.end(), subCmds.begin(), subCmds.end());
            }
            ++blockIdx;
        }
        BUG_CHECK(archSpec.getArchVectorSize() == blockIdx,
                  "The Tofino architecture requires %1% pipes (provided %2% pipes).",
                  archSpec.getArchVectorSize(), blockIdx);
    }
    return pipelineSequences;
}

TofinoProgramInfo::TofinoProgramInfo(const TofinoCompilerResult &compilerResult,
                                     std::vector<PipeInfo> inputPipes,
                                     std::map<int, gress_t> declIdToGress,
                                     std::map<int, size_t> declIdToPipe)
    : TofinoSharedProgramInfo(compilerResult, std::move(inputPipes), std::move(declIdToGress),
                              std::move(declIdToPipe)) {
    concolicMethodImpls.add(*SharedTofinoConcolic::getSharedTofinoConcolicMethodImpls());
    // Restrict egress port to be an allowed valid port.
    const auto *validPortsCond = getValidPortConstraint(getTargetOutputPortVar());
    pipelineSequence.emplace_back(Continuation::Guard(validPortsCond));
}

std::vector<Continuation::Command> TofinoProgramInfo::processDeclaration(
    const IR::Type_Declaration *typeDecl, size_t pipeIdx) const {
    // Get the architecture specification for this target.
    const auto &archSpec = getArchSpec();

    BUG_CHECK(pipeIdx < archSpec.getArchVectorSize(),
              "Current pipe index %1%  exceed the total amount of specified pipes (%2%).",
              archSpec.getArchVectorSize(), pipeIdx);

    // Collect parameters.
    const auto *applyBlock = typeDecl->to<IR::IApply>();
    if (applyBlock == nullptr) {
        TESTGEN_UNIMPLEMENTED("Constructed type %s of type %s not supported.", typeDecl,
                              typeDecl->node_type_name());
    }
    // Retrieve the current canonical pipe in the architecture spec using the pipe index.
    const auto *archMember = archSpec.getArchMember(pipeIdx);

    std::vector<Continuation::Command> cmds;
    if ((archMember->blockName == "IngressParserT")) {
        // Between the metadata that is prepended to the input packet to the ingress parser and
        // the actual packet there is also port metadata of size PORT_METADATA_SIZE.
        const auto *portMetadata = createTargetUninitialized(
            IR::Type_Bits::get(TofinoConstants::PORT_METADATA_SIZE), false);
        const auto *stmt = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
            "prepend_to_prog_header", {portMetadata}, IR::Type_Void::get(),
            new IR::ParameterList(
                {new IR::Parameter("hdr", IR::Direction::In,
                                   IR::Type_Bits::get(TofinoConstants::PORT_METADATA_SIZE))})));
        cmds.emplace_back(stmt);
        // Some pipes in Tofino prepend metadata to the input packet.
        // In this case, the intrinsic metadata corresponds to the third parameter.
        const auto *paramType = resolveProgramType(
            &getCompilerResult().getProgram(), new IR::Type_Name("ingress_intrinsic_metadata_t"));
        const auto *archPath = new IR::PathExpression(paramType, new IR::Path("*ig_intr_md"));
        // We synthesize an argument for the method call.
        // The input is the global architecture variable corresponding to the metadata.
        const auto *metaPrependStmt = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
            "prepend_to_prog_header", {archPath}, IR::Type_Void::get(),
            new IR::ParameterList({new IR::Parameter("hdr", IR::Direction::In, paramType)})));
        cmds.emplace_back(metaPrependStmt);
    }
    // Some pipes in Tofino prepend metadata to the input packet.
    // In this case, the intrinsic metadata corresponds to the third parameter.
    if ((archMember->blockName == "EgressParserT")) {
        const auto *paramType = resolveProgramType(
            &getCompilerResult().getProgram(), new IR::Type_Name("egress_intrinsic_metadata_t"));
        const auto *archPath = new IR::PathExpression(paramType, new IR::Path("*eg_intr_md"));
        // We synthesize an argument for the method call.
        // The input is the global architecture variable corresponding to the metadata.
        const auto *metaPrependStmt = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
            "prepend_to_prog_header", {archPath}, IR::Type_Void::get(),
            new IR::ParameterList({new IR::Parameter("hdr", IR::Direction::In, paramType)})));
        cmds.emplace_back(metaPrependStmt);
    }
    // Copy-in.
    const auto *copyInCall = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
        "copy_in", {IR::StringLiteral::get(typeDecl->name)}, IR::Type_Void::get(),
        new IR::ParameterList(
            {new IR::Parameter("blockRef", IR::Direction::In, IR::Type_Unknown::get())})));
    cmds.emplace_back(copyInCall);
    // Insert the actual pipeline.
    cmds.emplace_back(typeDecl);
    // Copy-out.
    const auto *copyOutCall = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
        "copy_out", {IR::StringLiteral::get(typeDecl->name)}, IR::Type_Void::get(),
        new IR::ParameterList(
            {new IR::Parameter("blockRef", IR::Direction::In, IR::Type_Unknown::get())})));
    cmds.emplace_back(copyOutCall);

    // After some specific pipelines (deparsers), we have to append the remaining packet
    // payload. We use an extern call for this.
    if (!((archMember->blockName == "IngressDeparserT") ||
          (archMember->blockName == "EgressDeparserT"))) {
        return cmds;
    }

    if ((archMember->blockName == "IngressDeparserT")) {
        const auto *nineBitType = IR::Type_Bits::get(9);
        // After the deparser, we store the ucast_egress_port as the device output port.
        auto *portAssign = new IR::AssignmentStatement(
            getTargetOutputPortVar(),
            new IR::Member(nineBitType, new IR::PathExpression("*ig_intr_md_for_tm"),
                           "ucast_egress_port"));
        cmds.emplace_back(portAssign);
    }

    const auto *stmt =
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("prepend_emit_buffer", {}));
    cmds.emplace_back(stmt);

    // Also check whether we need to drop the packet.
    auto *dropStmt =
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("check_tofino_drop", {}));
    cmds.emplace_back(dropStmt);

    if ((archMember->blockName == "IngressDeparserT")) {
        const auto *oneBitType = IR::Type_Bits::get(1);

        // Check whether ig_intr_md_for_tm.bypass_egress == 1w1
        // If bypass_egress is set, we skip the rest of the pipeline after the ingress deparser.
        const IR::Expression *skipCond =
            new IR::Equ(IR::Constant::get(oneBitType, 1),
                        new IR::Member(oneBitType, new IR::PathExpression("*ig_intr_md_for_tm"),
                                       "bypass_egress"));
        // Restrict egress port to be an allowed valid port.
        const auto *validPortsCond = getValidPortConstraint(getTargetOutputPortVar());
        skipCond = new IR::LAnd(validPortsCond, skipCond);
        const auto *skipCheck = new IR::IfStatement(skipCond, new IR::ExitStatement(), nullptr);
        cmds.emplace_back(skipCheck);
    }
    return cmds;
}

const IR::StateVariable &TofinoProgramInfo::getTargetInputPortVar() const {
    return *new IR::StateVariable(
        new IR::Member(IR::Type_Bits::get(TofinoConstants::PORT_BIT_WIDTH),
                       new IR::PathExpression("*ig_intr_md"), "ingress_port"));
}

const IR::StateVariable &TofinoProgramInfo::getTargetOutputPortVar() const {
    return *new IR::StateVariable(
        new IR::Member(IR::Type_Bits::get(TofinoConstants::PORT_BIT_WIDTH),
                       new IR::PathExpression("*eg_intr_md"), "egress_port"));
}

const IR::Expression *TofinoProgramInfo::dropIsActive() const {
    BUG("dropIsActive is not implemented for the Tofino extension. Please use the "
        "\"check_tofino_drop\" extern.");
}

const ArchSpec &TofinoProgramInfo::getArchSpec() const { return ARCH_SPEC; }

const ArchSpec TofinoProgramInfo::ARCH_SPEC = ArchSpec(
    "Switch"_cs,
    {
        // parser IngressParserT<H, M>(
        //     packet_in pkt,
        //     out H hdr,
        //     out M ig_md,
        //     @optional out ingress_intrinsic_metadata_t ig_intr_md,
        //     @optional out ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm,
        //     @optional out ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr);
        {"IngressParserT"_cs,
         {nullptr, "*ig_hdr"_cs, "*ig_md"_cs, "*ig_intr_md"_cs, "*ig_intr_md_for_tm"_cs,
          "*ig_intr_md_from_prsr"_cs}},
        // control IngressT<H, M>(
        //     inout H hdr,
        //     inout M ig_md,
        //     @optional in ingress_intrinsic_metadata_t ig_intr_md,
        //     @optional in ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr,
        //     @optional inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        //     @optional inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm);
        {"IngressT"_cs,
         {"*ig_hdr"_cs, "*ig_md"_cs, "*ig_intr_md"_cs, "*ig_intr_md_from_prsr"_cs,
          "*ig_intr_md_for_dprsr"_cs, "*ig_intr_md_for_tm"_cs}},
        // control IngressDeparserT<H, M>(
        //     packet_out pkt,
        //     inout H hdr,
        //     in M metadata,
        //     @optional in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        //     @optional in ingress_intrinsic_metadata_t ig_intr_md);
        {"IngressDeparserT"_cs,
         {nullptr, "*ig_hdr"_cs, "*ig_md"_cs, "*ig_intr_md_for_dprsr"_cs, "*ig_intr_md"_cs}},
        // parser EgressParserT<H, M>(
        //     packet_in pkt,
        //     out H hdr,
        //     out M eg_md,
        //     @optional out egress_intrinsic_metadata_t eg_intr_md,
        //     @optional out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr);
        {"EgressParserT"_cs,
         {nullptr, "*eg_hdr"_cs, "*eg_md"_cs, "*eg_intr_md"_cs, "*eg_intr_md_from_prsr"_cs,
          "*eg_intr_md_for_dprsr"_cs}},
        // control EgressT<H, M>(
        //     inout H hdr,
        //     inout M eg_md,
        //     @optional in egress_intrinsic_metadata_t eg_intr_md,
        //     @optional in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        //     @optional inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        //     @optional inout egress_intrinsic_metadata_for_output_port_t
        //     eg_intr_md_for_oport);
        {"EgressT"_cs,
         {"*eg_hdr"_cs, "*eg_md"_cs, "*eg_intr_md"_cs, "*eg_intr_md_from_prsr"_cs,
          "*eg_intr_md_for_dprsr"_cs, "*eg_intr_md_for_oport"_cs}},
        // control EgressDeparserT<H, M>(
        //     packet_out pkt,
        //     inout H hdr,
        //     in M metadata,
        //     @optional in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        //     @optional in egress_intrinsic_metadata_t eg_intr_md,
        //     @optional in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr);
        {"EgressDeparserT"_cs,
         {nullptr, "*eg_hdr"_cs, "*eg_md"_cs, "*eg_intr_md_for_dprsr"_cs, "*eg_intr_md"_cs,
          "*eg_intr_md_from_prsr"_cs}},
    });

}  // namespace P4::P4Tools::P4Testgen::Tofino
