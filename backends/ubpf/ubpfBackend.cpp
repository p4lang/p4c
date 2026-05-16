// Copyright 2019 Orange
// SPDX-FileCopyrightText: 2019 Orange
//
// SPDX-License-Identifier: Apache-2.0

#include "ubpfBackend.h"

#include "codeGen.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "lib/error.h"
#include "lib/nullstream.h"
#include "target.h"
#include "ubpfProgram.h"
#include "ubpfType.h"

namespace P4::UBPF {

void run_ubpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                      P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    if (toplevel == nullptr) return;

    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::P4::warning(ErrorType::WARN_MISSING,
                      "Could not locate top-level block; is there a %1% module?",
                      IR::P4Program::main);
        return;
    }

    UbpfTarget *target;
    if (options.target.isNullOrEmpty() || options.target == "ubpf") {
        target = new UbpfTarget();
    } else {
        ::P4::error(ErrorType::ERR_INVALID, "Unknown target %s; legal choice is 'ubpf'",
                    options.target);
        return;
    }

    UBPFTypeFactory::createFactory(typeMap);
    auto *prog = new UBPFProgram(options, toplevel->getProgram(), refMap, typeMap, toplevel);

    if (!prog->build()) return;

    if (options.outputFile.empty()) return;

    auto cstream = openFile(options.outputFile, false);
    if (cstream == nullptr) return;

    std::filesystem::path hfile = options.outputFile;
    hfile.replace_extension(".h");

    auto hstream = openFile(hfile, false);
    if (hstream == nullptr) return;

    UbpfCodeBuilder c(target);
    UbpfCodeBuilder h(target);

    prog->emitH(&h, hfile);
    prog->emitC(&c, hfile.filename());

    *cstream << c.toString();
    *hstream << h.toString();
    cstream->flush();
    hstream->flush();
}

}  // namespace P4::UBPF
