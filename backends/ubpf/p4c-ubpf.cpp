// Copyright 2019 Orange
// SPDX-FileCopyrightText: 2019 Orange
//
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include <iostream>
#include <string>

#include "backends/ebpf/ebpfOptions.h"
#include "backends/ubpf/midend.h"
#include "backends/ubpf/version.h"
#include "control-plane/p4RuntimeSerializer.h"
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
#include "ubpfBackend.h"
#include "ubpfModel.h"

using namespace P4;

void compile(EbpfOptions &options) {
    auto hook = options.getDebugHook();
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    if (isv1) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED, "- this compiler only handles P4-16");
        return;
    }
    auto program = P4::parseP4File(options);
    if (::P4::errorCount() > 0) return;

    P4::FrontEnd frontend;
    program = UBPF::UBPFModel::instance.run(program);
    frontend.addDebugHook(hook);
    program = frontend.run(options, program);
    if (::P4::errorCount() > 0) return;

    P4::serializeP4RuntimeIfRequired(program, options);
    if (::P4::errorCount() > 0) return;

    UBPF::MidEnd midend;
    midend.addDebugHook(hook);
    auto toplevel = midend.run(options, program);
    if (::P4::errorCount() > 0) return;

    UBPF::run_ubpf_backend(options, toplevel, &midend.refMap, &midend.typeMap);
}

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    AutoCompileContext autoEbpfContext(new EbpfContext);
    auto &options = EbpfContext::get().options();
    options.compilerVersion = cstring(P4C_UBPF_VERSION_STRING);

    if (options.process(argc, argv) != nullptr) {
        options.setInputFile();
    }

    if (::P4::errorCount() > 0) exit(1);

    try {
        compile(options);
    } catch (const std::exception &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }

    if (Log::verbose()) std::cout << "Done." << std::endl;

    return ::P4::errorCount() > 0;
}
