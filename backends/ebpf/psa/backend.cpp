// SPDX-FileCopyrightText: 2022 Open Networking Foundation
// SPDX-FileCopyrightText: 2022 Orange
//
// SPDX-License-Identifier: Apache-2.0
#include "backend.h"

#include "backends/common/psaProgramStructure.h"

namespace P4::EBPF {

void PSASwitchBackend::convert(const IR::ToplevelBlock *tlb) {
    CHECK_NULL(tlb);
    P4::PsaProgramStructure structure(refMap, typeMap);
    auto *parsePsaArch = new P4::ParsePsaArchitecture(&structure);
    const auto *main = tlb->getMain();
    if (main == nullptr) {
        return;
    }

    if (main->type->name != "PSA_Switch") {
        ::P4::warning(ErrorType::WARN_INVALID,
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

    EBPFTypeFactory::createFactory(typeMap, false);
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

}  // namespace P4::EBPF
