#include <stdio.h>
#include <string>
#include <iostream>

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/gc.h"

#include "midend.h"
#include "ebpfOptions.h"
#include "ebpfBackend.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"

void compile(EbpfOptions& options) {
    auto hook = options.getDebugHook();
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;
    if (isv1) {
        ::error("This compiler only handles P4 v1.2");
        return;
    }
    auto program = parseP4File(options);
    if (::errorCount() > 0)
        return;
    FrontEnd frontend;
    frontend.addDebugHook(hook);
    program = frontend.run(options, program);
    if (::errorCount() > 0)
        return;

    EBPF::MidEnd midend;
    midend.addDebugHook(hook);
    program = midend.run(options, program);
    if (::errorCount() > 0)
        return;

    EBPF::run_ebpf_backend(options, program);
}

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    EbpfOptions options;
    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        exit(1);

    compile(options);

    if (options.verbosity > 0)
        std::cerr << "Done." << std::endl;
    return ::errorCount() > 0;
}
