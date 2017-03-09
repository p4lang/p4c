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
#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/nullstream.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "midend.h"
#include "jsonconverter.h"

int main(int argc, char *const argv[]) {
    setup_gc_logging();

    CompilerOptions options;
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = "0.0.5";

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    auto hook = options.getDebugHook();

    // BMV2 is required for compatibility with the previous compiler.
    options.preprocessor_options += " -D__TARGET_BMV2__";
    auto program = parseP4File(options);
    if (program == nullptr || ::errorCount() > 0)
        return 1;
    P4::FrontEnd frontend;
    frontend.addDebugHook(hook);
    program = frontend.run(options, program);
    if (program == nullptr || ::errorCount() > 0)
        return 1;

    BMV2::MidEnd midEnd(options);
    midEnd.addDebugHook(hook);
    auto toplevel = midEnd.process(program);
    if (options.dumpJsonFile)
        JSONGenerator(*openFile(options.dumpJsonFile, true)) << program << std::endl;
    if (::errorCount() > 0 || toplevel == nullptr)
        return 1;

    BMV2::JsonConverter converter(options);
    converter.convert(&midEnd.refMap, &midEnd.typeMap, toplevel, &midEnd.enumMap);
    if (::errorCount() > 0)
        return 1;

    if (!options.outputFile.isNullOrEmpty()) {
        std::ostream* out = openFile(options.outputFile, false);
        if (out != nullptr) {
            converter.serialize(*out);
            out->flush();
        }
    }

    // Generate a PI control plane API for this program if requested.
    if (!options.p4RuntimeFile.isNullOrEmpty()) {
        std::ostream* out = openFile(options.p4RuntimeFile, false);
        if (out != nullptr) {
            auto format = options.p4RuntimeAsJson ? P4::P4RuntimeFormat::JSON
                                                  : P4::P4RuntimeFormat::BINARY;
            serializeP4Runtime(out, program, toplevel, &midEnd.refMap,
                               &midEnd.typeMap, format);
        }
    }

    return ::errorCount() > 0;
}
