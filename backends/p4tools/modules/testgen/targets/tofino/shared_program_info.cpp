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

#include "backends/p4tools/modules/testgen/targets/tofino/shared_program_info.h"

#include "backends/p4tools/common/lib/util.h"
#include "ir/irutils.h"

#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/tofino/compiler_result.h"
#include "backends/p4tools/modules/testgen/targets/tofino/constants.h"

namespace P4::P4Tools::P4Testgen::Tofino {

const IR::Type_Bits TofinoSharedProgramInfo::parserErrBits = IR::Type_Bits(16, false);

TofinoSharedProgramInfo::TofinoSharedProgramInfo(const TofinoCompilerResult &compilerResult,
                                                 std::vector<PipeInfo> inputPipes,
                                                 std::map<int, gress_t> declIdToGress,
                                                 std::map<int, size_t> declIdToPipe)
    : ProgramInfo(compilerResult),
      pipes(std::move(inputPipes)),
      declIdToGress(std::move(declIdToGress)),
      declIdToPipe(std::move(declIdToPipe)) {
    pipelineSequence.push_back(
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("StartIngress", {})));
    pipelineSequence.push_back(
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("StartEgress", {})));

    // The size of an input packet must at least be 64 bytes for the Tofino PTF model.
    targetConstraints =
        new IR::Geq(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                    IR::Constant::get(&PacketVars::PACKET_SIZE_VAR_TYPE,
                                      SharedTofinoConstants::PTF_INPUT_PKT_MIN_SIZE));
    // Map the block names to their canonical counter parts.
    for (const auto &pipeInfo : pipes) {
        for (const auto &declTuple : pipeInfo.pipes) {
            blockMap.emplace(declTuple.second->getName(), declTuple.first);
        }
    }
}

gress_t TofinoSharedProgramInfo::getGress(const IR::Type_Declaration *decl) const {
    return declIdToGress.at(decl->declid);
}

size_t TofinoSharedProgramInfo::getPipeIdx(const IR::Type_Declaration *decl) const {
    return declIdToPipe.at(decl->declid);
}

bool TofinoSharedProgramInfo::isMultiPipe() const { return pipes.size() > 1; }

const IR::P4Table *TofinoSharedProgramInfo::getTableofDirectExtern(
    const IR::IDeclaration *directExternDecl) const {
    const auto &directExternMap = getCompilerResult().getDirectExternMap();
    auto it = directExternMap.find(directExternDecl->controlPlaneName());
    if (it == directExternMap.end()) {
        return nullptr;
    }
    return it->second;
}

const std::vector<TofinoSharedProgramInfo::PipeInfo> *TofinoSharedProgramInfo::getPipes() const {
    return &pipes;
}

const IR::Type_Bits *TofinoSharedProgramInfo::getParserErrorType() const { return &parserErrBits; }

const IR::Expression *TofinoSharedProgramInfo::getBackendConstraint(
    const IR::StateVariable &portVar) {
    if (TestgenOptions::get().testBackend == "STF") {
        // The STF framework doesn't work well with multi-pipe. Some tests crash the
        // model, and control plane names differ from PTF - STF doesn't load bfrt.json,
        // only context.json for every pipe, which isn't enough to disabiguate conflicting
        // control plane names.
        return new IR::Lss(portVar, IR::Constant::get(portVar->type, 64));
    }
    if (TestgenOptions::get().testBackend == "PTF") {
        // TODO: by default, tofino model sets up 16 ports.
        return new IR::Lss(portVar, IR::Constant::get(portVar->type, 16));
    }
    return nullptr;
}

IR::StateVariable TofinoSharedProgramInfo::getParserParamVar(const IR::P4Parser *parser,
                                                             const IR::Type *type,
                                                             size_t paramIndex,
                                                             cstring paramLabel) const {
    auto gress = getGress(parser);

    // We need to explicitly map the parser error, if the parameter is set.
    // If the optional parser metadata parameter is not present, write directly to the
    // global parser metadata state. Otherwise, we retrieve the parameter name.
    auto parserApplyParams = parser->getApplyParameters()->parameters;
    cstring structLabel = nullptr;
    if (parserApplyParams.size() > paramIndex) {
        const auto *paramString = parserApplyParams.at(paramIndex);
        structLabel = paramString->name;
    } else {
        switch (gress) {
            case INGRESS: {
                structLabel = getArchSpec().getParamName("IngressParserT"_cs, paramIndex);
                break;
            }

            case EGRESS: {
                structLabel = getArchSpec().getParamName("EgressParserT"_cs, paramIndex);
                break;
            }

            case GHOST:
            default:
                BUG("Unimplemented thread: %1%", gress);
        }
    }
    return {new IR::Member(type, new IR::PathExpression(structLabel), paramLabel)};
}

const TofinoCompilerResult &TofinoSharedProgramInfo::getCompilerResult() const {
    return ProgramInfo::getCompilerResult().as<TofinoCompilerResult>();
}
}  // namespace P4::P4Tools::P4Testgen::Tofino
