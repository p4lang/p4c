#include <stdio.h>
#include <string>
#include <iostream>

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/gc.h"

#include "ebpfOptions.h"
#include "ebpfBackend.h"
#include "frontends/p4/p4-parse.h"
#include "frontends/p4/frontend.h"

void compile(EbpfOptions& options, FILE* in) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;
    if (isv1) {
        ::error("This compiler only handles P4 v1.2");
        return;
    }
    const IR::P4Program* v12 = parse_p4v1_2_file(options.file, in);
    options.closeInput(in);
    if (ErrorReporter::instance.getErrorCount() > 0) {
        ::error("%1% errors ecountered, aborting compilation",
                ErrorReporter::instance.getErrorCount());
        return;
    }
    FrontEnd frontend;
    v12 = frontend.run(options, v12);
    EBPF::run_ebpf_backend(options, v12);
}

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    EbpfOptions options;
    if (options.process(argc, argv) != nullptr)
        options.setInputFile();

    if (ErrorReporter::instance.getErrorCount() > 0)
        exit(1);

    FILE* in = options.preprocess();
    if (in != nullptr) {
        compile(options, in);
    }

    if (verbose)
        std::cerr << "Done." << std::endl;

    return ErrorReporter::instance.getErrorCount() > 0;
}
