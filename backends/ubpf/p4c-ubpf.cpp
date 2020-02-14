/*
Copyright 2019 Orange

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

#include <stdio.h>
#include <string>
#include <iostream>

#include "backends/ubpf/version.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/nullstream.h"

#include "backends/ubpf/midend.h"
#include "backends/ebpf/ebpfOptions.h"
#include "ubpfBackend.h"
#include "frontends/p4/frontend.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "ir/json_loader.h"
#include "fstream"

void compile(EbpfOptions& options) {
    auto hook = options.getDebugHook();
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    if (isv1) {
        ::error(ErrorType::ERR_UNSUPPORTED, "- this compiler only handles P4-16");
        return;
    }
    auto program = P4::parseP4File(options);
    if (::errorCount() > 0)
        return;

    P4::FrontEnd frontend;
    frontend.addDebugHook(hook);
    program = frontend.run(options, program);
    if (::errorCount() > 0)
        return;

    UBPF::MidEnd midend;
    midend.addDebugHook(hook);
    auto toplevel = midend.run(options, program);
    if (::errorCount() > 0)
        return;

    UBPF::run_ubpf_backend(options, toplevel, &midend.refMap, &midend.typeMap);
}


int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    AutoCompileContext autoEbpfContext(new EbpfContext);
    auto &options = EbpfContext::get().options();
    options.compilerVersion = P4C_UBPF_VERSION_STRING;

    if (options.process(argc, argv) != nullptr) {
        options.setInputFile();
    }

    if (::errorCount() > 0)
        exit(1);

    try {
        compile(options);
    } catch (const std::exception &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }

    if (Log::verbose())
        std::cout << "Done." << std::endl;

    return ::errorCount() > 0;
}

