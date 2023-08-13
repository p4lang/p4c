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
#include "backend.h"

#include <functional>
#include <ostream>
#include <set>

#include "../bmv2/common/lower.h"
#include "backends/dpdk/constants.h"
#include "backends/dpdk/dpdkProgramStructure.h"
#include "backends/dpdk/options.h"
#include "dpdkArch.h"
#include "dpdkAsmOpt.h"
#include "dpdkCheckExternInvocation.h"
#include "dpdkContext.h"
#include "dpdkHelpers.h"
#include "dpdkMetadata.h"
#include "dpdkProgram.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "lib/cstring.h"
#include "lib/null.h"
#include "lib/nullstream.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "midend/eliminateTypedefs.h"
#include "midend/removeComplexExpressions.h"
#include "midend/simplifyKey.h"
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

    std::set<const IR::P4Table *> invokedInKey;
    auto convertToDpdk = new ConvertToDpdkProgram(refMap, typeMap, &structure, options);
    auto genContextJson = new DpdkContextGenerator(refMap, &structure, p4info, options);
    bool is_all_args_header_fields = true;
    PassManager simplify = {
        new DpdkArchFirst(),
        new CollectLocalStructAndFlatten(refMap, typeMap),
        new P4::EliminateTypedef(refMap, typeMap),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new ByteAlignment(typeMap, refMap, &structure),
        new P4::SimplifyKey(
            refMap, typeMap,
            new P4::OrPolicy(new P4::IsValid(refMap, typeMap), new P4::IsLikeLeftValue())),
        new P4::TypeChecking(refMap, typeMap),
        // TBD: implement dpdk lowering passes instead of reusing bmv2's lowering pass.
        new PassRepeated({new BMV2::LowerExpressions(typeMap, DPDK_MAX_SHIFT_AMOUNT)}, 2),
        new P4::RemoveComplexExpressions(refMap, typeMap,
                                         new DPDK::ProcessControls(&structure.pipeline_controls)),
        new DismantleMuxExpressions(typeMap, refMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new ElimHeaderCopy(typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        new ConvertActionSelectorAndProfile(refMap, typeMap, &structure),
        new CollectTableInfo(&structure),
        new CollectAddOnMissTable(refMap, typeMap, &structure),
        new ValidateAddOnMissExterns(refMap, typeMap, &structure),
        new P4::MoveDeclarations(),  // Move all local declarations to the beginning
        new CollectProgramStructure(refMap, typeMap, &structure),
        new CollectMetadataHeaderInfo(&structure),
        new ConvertLookahead(refMap, typeMap, &structure),
        new P4::TypeChecking(refMap, typeMap),
        new ConvertToDpdkArch(refMap, &structure),
        new InjectJumboStruct(&structure),
        new InjectFixedMetadataField(&structure),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new P4::ResolveReferences(refMap),
        new DpdkHandleIPSec(refMap, typeMap, &structure),
        new StatementUnroll(refMap, &structure),
        new IfStatementUnroll(refMap),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new ConvertBinaryOperationTo2Params(refMap),
        new CollectProgramStructure(refMap, typeMap, &structure),
        new CopyMatchKeysToSingleStruct(refMap, typeMap, &invokedInKey, &structure),
        new P4::ResolveReferences(refMap),
        new CollectLocalVariables(refMap, typeMap, &structure),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new CollectErrors(&structure),
        new ConvertInternetChecksum(typeMap, &structure),
        new DefActionValue(typeMap, refMap, &structure),
        new PrependPDotToActionArgs(typeMap, refMap, &structure),
        new ConvertLogicalExpression(),
        new CollectExternDeclaration(&structure),
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new CollectDirectCounterMeter(refMap, typeMap, &structure),
        new ValidateDirectCounterMeter(refMap, typeMap, &structure),
        new DpdkAddPseudoHeader(refMap, typeMap, is_all_args_header_fields),
        new CollectProgramStructure(refMap, typeMap, &structure),
        new InspectDpdkProgram(refMap, typeMap, &structure),
        new CheckExternInvocation(refMap, typeMap, &structure),
        new TypeWidthValidator(),
        new DpdkArchLast(),
        new VisitFunctor([this, genContextJson] {
            // Serialize context json object into user specified file
            if (!options.ctxtFile.isNullOrEmpty()) {
                std::ostream *out = openFile(options.ctxtFile, false);
                if (out != nullptr) {
                    genContextJson->serializeContextJson(out);
                }
                out->flush();
            }
        }),
        new ReplaceHdrMetaField(),
        // convert to assembly program
        convertToDpdk,
    };
    simplify.addDebugHook(hook, true);
    program = program->apply(simplify);
    ordered_set<cstring> used_fields;
    dpdk_program = convertToDpdk->getDpdkProgram();
    if (!dpdk_program) return;
    if (structure.p4arch == "pna") {
        PassManager post_code_gen = {
            new PrependPassRecircId(),
            new DirectionToRegRead(),
        };
        dpdk_program = dpdk_program->apply(post_code_gen)->to<IR::DpdkAsmProgram>();
    }
    ordered_map<cstring, cstring> newNameMap;
    PassManager post_code_gen = {
        new EliminateUnusedAction(),
        new DpdkAsmOptimization,
        new CopyPropagationAndElimination(typeMap),
        new CollectUsedMetadataField(used_fields),
        new RemoveUnusedMetadataFields(used_fields),
        new ShortenTokenLength(newNameMap),
        new EmitDpdkTableConfig(refMap, typeMap, newNameMap),
    };

    dpdk_program = dpdk_program->apply(post_code_gen)->to<IR::DpdkAsmProgram>();
}

void DpdkBackend::codegen(std::ostream &out) const { dpdk_program->toSpec(out) << std::endl; }
}  // namespace DPDK
