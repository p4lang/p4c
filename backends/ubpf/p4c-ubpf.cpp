#include <stdio.h>
#include <string>
#include <iostream>

#include "backends/ebpf/version.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/nullstream.h"

#include "backends/ebpf/midend.h"
#include "backends/ebpf/ebpfOptions.h"
#include "ubpfBackend.h"
#include "frontends/p4/frontend.h"
#include "frontends/common/parseInput.h"
#include "ir/json_loader.h"
#include "fstream"

void compile(EbpfOptions& options) {
    auto hook = options.getDebugHook();
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    if (isv1) {
        ::error("This compiler only handles P4-16");
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

    EBPF::MidEnd midend;
    midend.addDebugHook(hook);
    auto toplevel = midend.run(options, program);
    if (options.dumpJsonFile)
        JSONGenerator(*openFile(options.dumpJsonFile, true)) << program << std::endl;
    if (::errorCount() > 0)
        return;

    UBPF::run_ubpf_backend(options, toplevel, &midend.refMap, &midend.typeMap);
}


int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    AutoCompileContext autoEbpfContext(new EbpfContext);
    auto& options = EbpfContext::get().options();
    options.compilerVersion = "0.0.1-beta";

    if (options.process(argc, argv) != nullptr) {
        options.setInputFile();
    }

    if(::errorCount() > 0)
        exit(1);

    try {
        compile(options);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }

    if(Log::verbose())
        std::cerr << "Done." << std::endl;

    return ::errorCount() > 0;

}

