// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "ebpfBackend.h"

#include <memory>

#include "ebpfProgram.h"
#include "ebpfType.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "lib/error.h"
#include "lib/nullstream.h"
#include "psa/backend.h"
#include "psa/ebpfPsaGen.h"
#include "target.h"

namespace P4::EBPF {

void emitFilterModel(const EbpfOptions &options, Target *target, const IR::ToplevelBlock *toplevel,
                     P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    CodeBuilder c(target);
    CodeBuilder h(target);

    EBPFTypeFactory::createFactory(typeMap, false);
    auto ebpfprog =
        std::make_unique<EBPFProgram>(options, toplevel->getProgram(), refMap, typeMap, toplevel);
    if (!ebpfprog->build()) return;

    if (options.outputFile.empty()) return;

    auto cstream = openFile(options.outputFile, false);
    if (cstream == nullptr) return;

    std::filesystem::path hfile = options.outputFile;
    hfile.replace_extension(".h");
    auto hstream = openFile(hfile, false);
    if (hstream == nullptr) return;

    ebpfprog->emitH(&h, hfile);
    ebpfprog->emitC(&c, hfile);
    *cstream << c.toString();
    *hstream << h.toString();
    cstream->flush();
    hstream->flush();
}

void run_ebpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                      P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    if (toplevel == nullptr) return;

    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::P4::warning(ErrorType::WARN_MISSING,
                      "Could not locate top-level block; is there a %1% module?",
                      IR::P4Program::main);
        return;
    }

    // We don't require --xdp option to be used if we can auto-detect it.
    bool mainIsXdp = (main->type->name == "xdp");

    std::unique_ptr<Target> target;
    if (options.target.isNullOrEmpty() || options.target == "kernel") {
        if (!options.generateToXDP && !mainIsXdp)
            target = std::make_unique<KernelSamplesTarget>(options.emitTraceMessages);
        else
            target = std::make_unique<XdpTarget>(options.emitTraceMessages);
    } else if (options.target == "bcc") {
        target = std::make_unique<BccTarget>();
    } else if (options.target == "test") {
        target = std::make_unique<TestTarget>();
    } else {
        ::P4::error(ErrorType::ERR_UNKNOWN,
                    "Unknown target %s; legal choices are 'bcc', 'kernel', and test",
                    options.target);
        return;
    }

    if (options.arch.isNullOrEmpty() || options.arch == "filter") {
        emitFilterModel(options, target.get(), toplevel, refMap, typeMap);
    } else if (options.arch == "psa") {
        auto backend =
            std::make_unique<EBPF::PSASwitchBackend>(options, target.get(), refMap, typeMap);
        backend->convert(toplevel);

        if (options.outputFile.empty()) return;

        if (auto cstream = openFile(options.outputFile, false)) {
            backend->codegen(*cstream);
            cstream->flush();
        }
    } else {
        ::P4::error(ErrorType::ERR_UNKNOWN,
                    "Unknown architecture %s; legal choices are 'filter', and 'psa'", options.arch);
        return;
    }
}

}  // namespace P4::EBPF
