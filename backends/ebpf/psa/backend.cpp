/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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

#include "backends/common/psaProgramStructure.h"

namespace p4c::EBPF {

void PSASwitchBackend::convert(const IR::ToplevelBlock *tlb) {
    CHECK_NULL(tlb);
    P4::PsaProgramStructure structure(refMap, typeMap);
    auto *parsePsaArch = new P4::ParsePsaArchitecture(&structure);
    const auto *main = tlb->getMain();
    if (main == nullptr) {
        return;
    }

    if (main->type->name != "PSA_Switch") {
        ::warning(ErrorType::WARN_INVALID,
                  "%1%: the main package should be called PSA_Switch"
                  "; are you using the wrong architecture?",
                  main->type->name);
        return;
    }

    main->apply(*parsePsaArch);
    auto *evaluator = new P4::EvaluatorPass(refMap, typeMap);
    const auto *program = tlb->getProgram();

    PassManager rewriteToEBPF = {
        evaluator,
        new VisitFunctor(
            [this, evaluator, structure]() { toplevel = evaluator->getToplevelBlock(); }),
    };

    auto hook = options.getDebugHook();
    rewriteToEBPF.addDebugHook(hook, true);
    program = program->apply(rewriteToEBPF);

    // map IR node to compile-time allocated resource blocks.
    toplevel->apply(*new P4::BuildResourceMap(&structure.resourceMap));

    main = toplevel->getMain();
    if (main == nullptr) {
        return;  // no main
    }
    main->apply(*parsePsaArch);
    program = toplevel->getProgram();

    EBPFTypeFactory::createFactory(typeMap);
    auto *convertToEbpfPSA = new ConvertToEbpfPSA(options, refMap, typeMap);
    PassManager toEBPF = {
        new P4::DiscoverStructure(&structure),
        new P4::InspectPsaProgram(refMap, typeMap, &structure),
        // convert to EBPF objects
        new VisitFunctor([evaluator, convertToEbpfPSA]() {
            auto *tlb = evaluator->getToplevelBlock();
            tlb->apply(*convertToEbpfPSA);
        }),
    };

    toEBPF.addDebugHook(hook, true);
    program = program->apply(toEBPF);

    ebpf_program = convertToEbpfPSA->getPSAArchForEBPF();
}

}  // namespace p4c::EBPF
