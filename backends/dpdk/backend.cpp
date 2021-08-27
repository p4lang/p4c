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
#include "midend/eliminateTypedefs.h"
#include "ir/dbprint.h"
#include "ir/ir.h"
#include "lib/stringify.h"

namespace DPDK {

class DpdkArchFirst : public PassManager {
 public:
    DpdkArchFirst() { setName("DpdkArchFirst"); }
};

void DpdkBackend::convert(const IR::ToplevelBlock *tlb) {
    CHECK_NULL(tlb);
    DpdkProgramStructure structure;
    auto parseDpdkArch = new ParseDpdkArchitecture(&structure);
    auto main = tlb->getMain();
    if (!main) return;
    main->apply(*parseDpdkArch);

    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto program = tlb->getProgram();

    auto rewriteToDpdkArch =
        new DPDK::RewriteToDpdkArch(refMap, typeMap, &structure);
    PassManager simplify = {
        new DpdkArchFirst(),
        new P4::EliminateTypedef(refMap, typeMap),
        // because the user metadata type has changed
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap),
        // TBD: implement dpdk lowering passes instead of reusing bmv2's
        // lowering pass.
        new BMV2::LowerExpressions(typeMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new P4::TypeChecking(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        // Convert to Dpdk specific format
        rewriteToDpdkArch,
        new P4::ClearTypeMap(typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        evaluator,
        new VisitFunctor([this, evaluator, structure]() {
            toplevel = evaluator->getToplevelBlock();
        }),
    };
    auto hook = options.getDebugHook();
    simplify.addDebugHook(hook, true);
    program = program->apply(simplify);

    main = toplevel->getMain();
    if (!main) return;
    main->apply(*parseDpdkArch);

    program = toplevel->getProgram();

    auto convertToDpdk = new ConvertToDpdkProgram(refMap, typeMap, &structure);
    PassManager toAsm = {
        new InspectDpdkProgram(refMap, typeMap, &structure),
        // convert to assembly program
        convertToDpdk,
    };
    toAsm.addDebugHook(hook, true);
    program = program->apply(toAsm);
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
