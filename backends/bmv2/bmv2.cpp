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

#include <stdio.h>
#include <string>
#include <iostream>

#include "ir/ir.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "backend.h"
#include "midend.h"
#include "JsonObjects.h"

int main(int argc, char *const argv[]) {
    setup_gc_logging();

    AutoCompileContext autoBMV2Context(new BMV2::BMV2Context);
    auto& options = BMV2::BMV2Context::get().options();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = "0.0.5";

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    auto hook = options.getDebugHook();

    // BMV2 is required for compatibility with the previous compiler.
    options.preprocessor_options += " -D__TARGET_BMV2__";
    auto program = P4::parseP4File(options);
    if (program == nullptr || ::errorCount() > 0)
        return 1;
    try {
        P4::P4COptionPragmaParser optionsPragmaParser;
        program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

        P4::FrontEnd frontend;
        frontend.addDebugHook(hook);
        program = frontend.run(options, program);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (program == nullptr || ::errorCount() > 0)
        return 1;

    const IR::ToplevelBlock* toplevel = nullptr;
    BMV2::MidEnd midEnd(options);
    midEnd.addDebugHook(hook);
    try {
        toplevel = midEnd.process(program);
        if (::errorCount() > 1 || toplevel == nullptr ||
            toplevel->getMain() == nullptr)
            return 1;
        if (options.dumpJsonFile)
            JSONGenerator(*openFile(options.dumpJsonFile, true)) << program << std::endl;
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0)
        return 1;

    // backend depends on the modified refMap and typeMap from midEnd.
    BMV2::Backend backend(options, &midEnd.refMap, &midEnd.typeMap, &midEnd.enumMap);

    if (auto arch = getenv("P4C_DEFAULT_ARCH")) {
        if (!strncmp(arch, "v1model", 7))
            backend.target = BMV2::Target::SIMPLE_SWITCH;
        else if (!strncmp(arch, "psa", 3))
            backend.target = BMV2::Target::PORTABLE_SWITCH;
    } else {
        if (options.arch == "v1model")
            backend.target = BMV2::Target::SIMPLE_SWITCH;
        else if (options.arch == "psa")
            backend.target = BMV2::Target::PORTABLE_SWITCH;
    }

    try {
        if (backend.target == BMV2::Target::SIMPLE_SWITCH) {
            backend.convert_simple_switch(toplevel, options);
        } else if (backend.target == BMV2::Target::PORTABLE_SWITCH) {
            backend.convert_portable_switch(toplevel, options);
        }
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0)
        return 1;

    if (!options.outputFile.isNullOrEmpty()) {
        std::ostream* out = openFile(options.outputFile, false);
        if (out != nullptr) {
            backend.serialize(*out);
            out->flush();
        }
    }

    P4::serializeP4RuntimeIfRequired(program, options);

    return ::errorCount() > 0;
}
