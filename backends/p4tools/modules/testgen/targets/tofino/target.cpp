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

#include "backends/p4tools/modules/testgen/targets/tofino/target.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/util.h"
// TOOD: We can only use this pass because it is header-only.
#include "backends/tofino/bf-p4c/midend/rewrite_egress_intrinsic_metadata_header.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/targets/tofino/compiler_result.h"
#include "backends/p4tools/modules/testgen/targets/tofino/map_direct_externs.h"
#include "backends/p4tools/modules/testgen/targets/tofino/rename_keys.h"
#include "backends/p4tools/modules/testgen/targets/tofino/shared_program_info.h"
#include "backends/p4tools/modules/testgen/targets/tofino/test_backend.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino/program_info.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino2/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino2/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/tofino/tofino2/program_info.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/* =============================================================================================
 *  AbstractTofinoTestgenTarget implementation
 * ============================================================================================= */

AbstractTofinoTestgenTarget::AbstractTofinoTestgenTarget(const std::string &deviceName,
                                                         const std::string &archName)
    : TestgenTarget(deviceName, archName) {}

TofinoTestBackend *AbstractTofinoTestgenTarget::getTestBackendImpl(
    const ProgramInfo &programInfo, const TestBackendConfiguration &testBackendConfiguration,
    SymbolicExecutor &symbex) const {
    return new TofinoTestBackend(*programInfo.checkedTo<TofinoSharedProgramInfo>(),
                                 testBackendConfiguration, symbex);
}

P4::FrontEnd AbstractTofinoTestgenTarget::mkFrontEnd() const {
    // FIXME: Reenable this.
    // return P4::FrontEnd(BFN::ParseAnnotations());
    return {};
}

MidEnd AbstractTofinoTestgenTarget::mkMidEnd(const CompilerOptions &options) const {
    // We need to initialize the device to be able to use Tofino compiler passes
    // FIXME: Reenable this?
    Device::init(spec.deviceName, {});
    auto midEnd = CompilerTarget::mkMidEnd(options);
    auto *refMap = midEnd.getRefMap();
    auto *typeMap = midEnd.getTypeMap();
    midEnd.addPasses({
        // Remove all unused fields in the egress intrinsic metadata
        // FIXME: Reenable this.
        new BFN::RewriteEgressIntrinsicMetadataHeader(refMap, typeMap),
        // Remove trailing '$' in '$valid$' key matches and
        // replace subscript operator in header stack indices
        // with '$'
        new RenameKeys(),
    });
    return midEnd;
}

CompilerResultOrError AbstractTofinoTestgenTarget::runCompilerImpl(
    const CompilerOptions &options, const IR::P4Program *program) const {
    program = runFrontend(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }

    if (errorCount() > 0) {
        return std::nullopt;
    }

    program = runMidEnd(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }

    // Create DCG.
    NodesCallGraph *dcg = nullptr;
    if (TestgenOptions::get().dcg || !TestgenOptions::get().pattern.empty()) {
        dcg = new NodesCallGraph("NodesCallGraph");
        P4ProgramDCGCreator dcgCreator(dcg);
        program->apply(dcgCreator);
    }
    if (errorCount() > 0) {
        return std::nullopt;
    }
    /// Collect coverage information about the program.
    auto coverage = P4::Coverage::CollectNodes(TestgenOptions::get().coverageOptions);
    program->apply(coverage);
    if (errorCount() > 0) {
        return std::nullopt;
    }

    // Try to map all instances of direct externs to the table they are attached to.
    // Save the map in @var directExternMap.
    auto directExternMapper = MapDirectExterns();
    program->apply(directExternMapper);
    if (errorCount() > 0) {
        return std::nullopt;
    }

    return {*new TofinoCompilerResult(
        TestgenCompilerResult(CompilerResult(*program), coverage.getCoverableNodes(), dcg),
        directExternMapper.getDirectExternMap())};
}

/* =============================================================================================
 *  Tofino_TnaTestgenTarget implementation
 * ============================================================================================= */

Tofino_TnaTestgenTarget::Tofino_TnaTestgenTarget() : AbstractTofinoTestgenTarget("tofino", "tna") {}

void Tofino_TnaTestgenTarget::make() {
    static Tofino_TnaTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Tofino_TnaTestgenTarget();
    }
}

const TofinoProgramInfo *Tofino_TnaTestgenTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // Ensure that main instantiates Switch.
    // TODO: Handle MultiParserSwitch.
    const auto *mainType = mainDecl->type->to<IR::Type_Specialized>();
    if ((mainType == nullptr) || mainType->baseType->path->name != "Switch") {
        return nullptr;
    }

    // The pipes in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of pipes, represented as constructor-call
    // expressions.
    const auto *mainDeclArgs = mainDecl->arguments;
    const auto *ns = NamespaceContext::Empty->push(&compilerResult.getProgram());
    std::vector<TofinoProgramInfo::PipeInfo> pipeInfos;
    std::map<int, gress_t> declIdToGress;
    std::map<int, size_t> declIdToPipe;

    // Expand each pipe into its constituent parser, MAU, and deparser, each represented as a
    // constructor-call expression. Also identify the gress for each parser, MAU control, and
    // deparser.
    // TODO: Handle ghost threads.
    for (size_t pipeIdx = 0; pipeIdx < mainDeclArgs->size(); ++pipeIdx) {
        const auto *pipe = mainDeclArgs->at(pipeIdx);

        std::vector<const IR::Type_Declaration *> blocks;
        const auto *const pathExpr = pipe->expression->checkedTo<IR::PathExpression>();
        const auto *declInstance =
            ns->findDecl(pathExpr->path)->checkedTo<IR::Declaration_Instance>();
        auto subBlocks =
            argumentsToTypeDeclarations(&compilerResult.getProgram(), declInstance->arguments);
        blocks.insert(blocks.end(), subBlocks.begin(), subBlocks.end());
        ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

        // We should have six arguments.
        BUG_CHECK(blocks.size() == 6, "%1%: Not a valid pipeline instantiation.", pipe);

        // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
        for (size_t idx = 0; idx < blocks.size(); ++idx) {
            const auto *declType = blocks.at(idx);

            auto canonicalName = TofinoProgramInfo::ARCH_SPEC.getArchMember(idx)->blockName;
            programmableBlocks.emplace(canonicalName, declType);

            if (idx < 3) {
                declIdToGress[declType->declid] = INGRESS;
            } else {
                declIdToGress[declType->declid] = EGRESS;
            }
            declIdToPipe[declType->declid] = pipeIdx;
        }
        pipeInfos.push_back({declInstance->controlPlaneName(), programmableBlocks});
    }
    return new TofinoProgramInfo(*compilerResult.checkedTo<TofinoCompilerResult>(), pipeInfos,
                                 declIdToGress, declIdToPipe);
}

TofinoCmdStepper *Tofino_TnaTestgenTarget::getCmdStepperImpl(ExecutionState &state,
                                                             AbstractSolver &solver,
                                                             const ProgramInfo &programInfo) const {
    return new TofinoCmdStepper(state, solver, programInfo);
}

Tofino1ExprStepper *Tofino_TnaTestgenTarget::getExprStepperImpl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new Tofino1ExprStepper(state, solver, programInfo);
}

/* =============================================================================================
 *  JBay_T2naTestgenTarget implementation
 * ============================================================================================= */

JBay_T2naTestgenTarget::JBay_T2naTestgenTarget() : AbstractTofinoTestgenTarget("tofino2", "t2na") {}

void JBay_T2naTestgenTarget::make() {
    static JBay_T2naTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new JBay_T2naTestgenTarget();
    }
}

const JBayProgramInfo *JBay_T2naTestgenTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // Ensure that main instantiates Switch.
    // TODO: Handle MultiParserSwitch.
    const auto *mainType = mainDecl->type->to<IR::Type_Specialized>();
    if ((mainType == nullptr) || mainType->baseType->path->name != "Switch") {
        return nullptr;
    }

    // The pipes in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of pipes, represented as constructor-call
    // expressions.
    const auto *mainDeclArgs = mainDecl->arguments;
    const auto *ns = NamespaceContext::Empty->push(&compilerResult.getProgram());
    std::vector<TofinoProgramInfo::PipeInfo> pipeInfos;
    std::map<int, gress_t> declIdToGress;
    std::map<int, size_t> declIdToPipe;

    // Expand each pipe into its constituent parser, MAU, and deparser, each represented as a
    // constructor-call expression. Also identify the gress for each parser, MAU control, and
    // deparser.
    // TODO: Handle ghost threads.
    for (size_t pipeIdx = 0; pipeIdx < mainDeclArgs->size(); ++pipeIdx) {
        const auto *pipe = mainDeclArgs->at(pipeIdx);

        std::vector<const IR::Type_Declaration *> blocks;
        const auto *const pathExpr = pipe->expression->checkedTo<IR::PathExpression>();
        const auto *declInstance =
            ns->findDecl(pathExpr->path)->checkedTo<IR::Declaration_Instance>();
        auto subBlocks =
            argumentsToTypeDeclarations(&compilerResult.getProgram(), declInstance->arguments);
        blocks.insert(blocks.end(), subBlocks.begin(), subBlocks.end());

        ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

        // We should have six or seven (in case of the optional ghost) arguments per pipeline.
        BUG_CHECK(blocks.size() == 6 || blocks.size() == 7,
                  "%1%: Not a valid pipeline instantiation. Has %2% blocks.", pipe, blocks.size());

        // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
        for (size_t idx = 0; idx < blocks.size(); ++idx) {
            const auto *declType = blocks.at(idx);

            auto canonicalName = JBayProgramInfo::ARCH_SPEC.getArchMember(idx)->blockName;
            programmableBlocks.emplace(canonicalName, declType);

            if (idx < 3) {
                declIdToGress[declType->declid] = INGRESS;
            } else if (idx < 6) {
                declIdToGress[declType->declid] = EGRESS;
            } else if (idx == 6) {
                declIdToGress[declType->declid] = GHOST;
            } else {
                BUG("Unexpected number of blocks %1%", idx);
            }
            declIdToPipe[declType->declid] = pipeIdx;
        }
        pipeInfos.push_back({declInstance->controlPlaneName(), programmableBlocks});
    }

    return new JBayProgramInfo(*compilerResult.checkedTo<TofinoCompilerResult>(), pipeInfos,
                               declIdToGress, declIdToPipe);
}

JBayCmdStepper *JBay_T2naTestgenTarget::getCmdStepperImpl(ExecutionState &state,
                                                          AbstractSolver &solver,
                                                          const ProgramInfo &programInfo) const {
    return new JBayCmdStepper(state, solver, programInfo);
}

JBayExprStepper *JBay_T2naTestgenTarget::getExprStepperImpl(ExecutionState &state,
                                                            AbstractSolver &solver,
                                                            const ProgramInfo &programInfo) const {
    return new JBayExprStepper(state, solver, programInfo);
}

}  // namespace P4::P4Tools::P4Testgen::Tofino
