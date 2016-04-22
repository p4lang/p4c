#include <stdio.h>
#include <string>
#include <iostream>

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/frontend.h"
#include "midend.h"

void printPass(const char* manager, unsigned seq, const char* pass, const IR::Node*)
{ std::cout << manager << ", pass #" << seq << ": " << pass << std::endl; }

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
    if (program != nullptr && ::errorCount() == 0) {
        FrontEnd fe;
        if (::verbose)
            fe.addDebugHook(printPass);
        program = fe.run(options, program);
        if (program != nullptr && ::errorCount() == 0) {
            V12Test::MidEnd midEnd;
            if (::verbose)
                midEnd.addDebugHook(printPass);
            (void)midEnd.process(options, program);
        }
    }
    if (verbose)
        std::cerr << "Done." << std::endl;
    return ::errorCount() > 0;
}
