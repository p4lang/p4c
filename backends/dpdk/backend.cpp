/*
Copyright 2020 Intel Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <unordered_map>
#include "backend.h"
#include "dpdkArch.h"
#include "dpdkAsmOpt.h"
#include "dpdkHelpers.h"
#include "dpdkProgram.h"
#include "frontends/p4/moveDeclarations.h"
#include "midend/eliminateTypedefs.h"
#include "midend/removeComplexExpressions.h"
#include "ir/dbprint.h"
#include "ir/ir.h"
#include "lib/stringify.h"
#include "../bmv2/common/lower.h"

namespace DPDK {

void DpdkBackend::convert(const IR::ToplevelBlock *tlb) {
    CHECK_NULL(tlb);
    DpdkProgramStructure structure;
    auto parseDpdkArch = new ParseDpdkArchitecture(&structure);
    auto main = tlb->getMain();
    if (!main) return;
    main->apply(*parseDpdkArch);

    auto hook = options.getDebugHook();
    auto program = tlb->getProgram();

    std::set<const IR::P4Table*> invokedInKey;
    auto convertToDpdk = new ConvertToDpdkProgram(refMap, typeMap, &structure);
    PassManager simplify = {
        new DpdkArchFirst(),
        new P4::EliminateTypedef(refMap, typeMap),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap),
        // TBD: implement dpdk lowering passes instead of reusing bmv2's lowering pass.
        new BMV2::LowerExpressions(typeMap),
        new P4::RemoveComplexExpressions(refMap, typeMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new P4::TypeChecking(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        new ConvertActionSelectorAndProfile(refMap, typeMap),
        new CollectAddOnMissTable(refMap, typeMap, &structure),
        new P4::MoveDeclarations(),  // Move all local declarations to the beginning
        new CollectProgramStructure(refMap, typeMap, &structure),
        new CollectMetadataHeaderInfo(&structure),
        new ConvertToDpdkArch(refMap, &structure),
        new InjectJumboStruct(&structure),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new CopyMatchKeysToSingleStruct(refMap, typeMap, &invokedInKey),
        new P4::ResolveReferences(refMap),
        new StatementUnroll(refMap, &structure),
        new IfStatementUnroll(refMap, &structure),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new ConvertBinaryOperationTo2Params(),
        new CollectProgramStructure(refMap, typeMap, &structure),
        new CollectLocalVariableToMetadata(refMap, &structure),
        new CollectErrors(&structure),
        new ConvertInternetChecksum(typeMap, &structure),
        new PrependPDotToActionArgs(typeMap, refMap, &structure),
        new ConvertLogicalExpression(),
        new CollectExternDeclaration(&structure),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new CollectProgramStructure(refMap, typeMap, &structure),
        new InspectDpdkProgram(refMap, typeMap, &structure),
        new DpdkArchLast(),
        // convert to assembly program
        convertToDpdk,
    };
    simplify.addDebugHook(hook, true);
    program = program->apply(simplify);

    dpdk_program = convertToDpdk->getDpdkProgram();
    if (!dpdk_program)
        return;
    PassManager post_code_gen = {
        new EliminateUnusedAction(),
        new DpdkAsmOptimization,
    };

    dpdk_program = dpdk_program->apply(post_code_gen)->to<IR::DpdkAsmProgram>();
}

void DpdkBackend::codegen(std::ostream &out) const {
    dpdk_program->toSpec(out) << std::endl;
}

}  // namespace DPDK
