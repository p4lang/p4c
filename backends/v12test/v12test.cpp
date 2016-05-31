/*
Copyright 2013-present Barefoot Networks, Inc. 

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
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;

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
