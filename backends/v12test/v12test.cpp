#include "ir/ir.h"
#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/toP4/toP4.h"
#include "midend.h"

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    CompilerOptions options;
    options.langVersion = CompilerOptions::FrontendVersion::P4v1_2;

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    auto program = parseP4File(options);
    auto hook = options.getDebugHook();

    if (program != nullptr && ::errorCount() == 0) {
        FrontEnd fe;
        fe.addDebugHook(hook);
        program = fe.run(options, program);
        if (program != nullptr && ::errorCount() == 0) {
            V12Test::MidEnd midEnd;
            midEnd.addDebugHook(hook);
            (void)midEnd.process(options, program);
        }
    }
    if (verbose)
        std::cerr << "Done." << std::endl;
    return ::errorCount() > 0;
}
