#include <stdio.h>
#include <gc/gc_cpp.h>

#include <string>
#include <iostream>

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/frontend.h"

int main(int argc, char *const argv[]) {
    setup_gc_logging();

    CompilerOptions options;
    options.langVersion = CompilerOptions::FrontendVersion::P4v1_2;

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    bool v1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;
    auto program = parseP4File(options);
    if (program != nullptr) {
        program = run_frontend(options, program, v1);
        if (program != nullptr) {
            // mid-end: just evaluate
            P4::EvaluatorPass evaluator(v1);
            program = program->apply(evaluator);
            if (::errorCount() == 0) {
                // triggers warning if there is no 'main'
                (void)evaluator.getBlockMap()->getMain();
            }
        }
    }
    if (verbose)
        std::cerr << "Done." << std::endl;
    return ::errorCount() > 0;
}
