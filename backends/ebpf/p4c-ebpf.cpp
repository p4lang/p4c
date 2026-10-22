// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include <iostream>
#include <string>

#include "backends/ebpf/version.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "ebpfBackend.h"
#include "ebpfOptions.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "fstream"
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "midend.h"

using namespace P4;

void compile(EbpfOptions &options) {
    auto hook = options.getDebugHook();
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    if (isv1) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "This compiler only handles P4-16");
        return;
    }
    IR::Ptr<IR::P4Program> program = nullptr;

    if (options.loadIRFromJson) {
        std::filebuf fb;
        if (fb.open(options.file, std::ios::in) == nullptr) {
            ::P4::error(ErrorType::ERR_IO, "%s: No such file or directory.", options.file);
            return;
        }

        std::istream inJson(&fb);
        JSONLoader jsonFileLoader(inJson);
        if (!jsonFileLoader) {
            ::P4::error(ErrorType::ERR_IO, "%s: Not valid input file", options.file);
            return;
        }
        program = new IR::P4Program(jsonFileLoader);
        fb.close();
    } else {
        program = P4::parseP4File(options);
        if (::P4::errorCount() > 0) return;

        P4::P4COptionPragmaParser optionsPragmaParser(true);
        program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

        P4::FrontEnd frontend;
        frontend.addDebugHook(hook);
        program = frontend.run(options, program);
        if (::P4::errorCount() > 0) return;
    }

    if (!options.arch.isNullOrEmpty() && options.arch != "filter") {
        P4::serializeP4RuntimeIfRequired(program, options);
        if (::P4::errorCount() > 0) return;
    }

    EBPF::MidEnd midend;
    midend.addDebugHook(hook);
    auto toplevel = midend.run(options, program);
    if (!options.dumpJsonFile.empty()) {
        auto dumpJsonStream = openFile(options.dumpJsonFile, true);
        JSONGenerator(*dumpJsonStream).emit(program);
    }
    if (::P4::errorCount() > 0) return;

    EBPF::run_ebpf_backend(options, toplevel, &midend.refMap, &midend.typeMap);
}

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    AutoCompileContext autoEbpfContext(new EbpfContext);
    auto &options = EbpfContext::get().options();
    options.compilerVersion = cstring(P4C_EBPF_VERSION_STRING);

    if (options.process(argc, argv) != nullptr) {
        if (options.loadIRFromJson == false) options.setInputFile();
    }
    if (::P4::errorCount() > 0) exit(1);

    options.calculateXDP2TCMode();
    try {
        compile(options);
    } catch (const std::exception &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }

    if (Log::verbose()) std::cerr << "Done." << std::endl;
    return ::P4::errorCount() > 0;
}
