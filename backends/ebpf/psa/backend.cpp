#include "backend.h"

#include "backends/bmv2/psa_switch/psaSwitch.h"

namespace EBPF {

void PSASwitchBackend::convert(const IR::ToplevelBlock *tlb) {
    CHECK_NULL(tlb);
    BMV2::PsaProgramStructure structure(refMap, typeMap);
    auto parsePsaArch = new BMV2::ParsePsaArchitecture(&structure);
    auto main = tlb->getMain();
    if (!main)
        return;

    if (main->type->name != "PSA_Switch") {
        ::warning(ErrorType::WARN_INVALID,
                  "%1%: the main package should be called PSA_Switch"
                  "; are you using the wrong architecture?",
                  main->type->name);
        return;
    }

    main->apply(*parsePsaArch);
    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto program = tlb->getProgram();

    PassManager rewriteToEBPF = {
        evaluator,
        new VisitFunctor([this, evaluator, structure]() {
            toplevel = evaluator->getToplevelBlock();
        }),
    };

    auto hook = options.getDebugHook();
    rewriteToEBPF.addDebugHook(hook, true);
    program = program->apply(rewriteToEBPF);

    // map IR node to compile-time allocated resource blocks.
    toplevel->apply(*new BMV2::BuildResourceMap(&structure.resourceMap));

    main = toplevel->getMain();
    if (!main)
        return;  // no main
    main->apply(*parsePsaArch);
    program = toplevel->getProgram();

    EBPFTypeFactory::createFactory(typeMap);
    auto convertToEbpfPSA = new ConvertToEbpfPSA(options, structure, refMap, typeMap);
    PassManager toEBPF = {
            new BMV2::DiscoverStructure(&structure),
            new BMV2::InspectPsaProgram(refMap, typeMap, &structure),
            // convert to EBPF objects
            new VisitFunctor([evaluator, convertToEbpfPSA]() {
                auto tlb = evaluator->getToplevelBlock();
                tlb->apply(*convertToEbpfPSA);
            }),
    };

    toEBPF.addDebugHook(hook, true);
    program = program->apply(toEBPF);

    ebpf_program = convertToEbpfPSA->getPSAArchForEBPF();
}

}  // namespace EBPF

