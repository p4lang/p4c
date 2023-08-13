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

#include <exception>
#include <iostream>

#include "backends/bmv2/common/options.h"
#include "backends/bmv2/psa_switch/midend.h"
#include "backends/bmv2/psa_switch/options.h"
#include "backends/bmv2/psa_switch/psaSwitch.h"
#include "backends/bmv2/psa_switch/version.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "fstream"
#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"
#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/gc.h"
#include "lib/nullstream.h"

int main(int argc, char *const argv[]) {
    setup_gc_logging();

    AutoCompileContext autoPsaSwitchContext(new BMV2::PsaSwitchContext);
    auto &options = BMV2::PsaSwitchContext::get().options();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = BMV2_PSA_VERSION_STRING;

    if (options.process(argc, argv) != nullptr) {
        if (options.loadIRFromJson == false) options.setInputFile();
    }
    if (::errorCount() > 0) return 1;

    auto hook = options.getDebugHook();

    // BMV2 is required for compatibility with the previous compiler.
    options.preprocessor_options += " -D__TARGET_BMV2__";

    const IR::P4Program *program = nullptr;
    const IR::ToplevelBlock *toplevel = nullptr;

    if (options.loadIRFromJson == false) {
        program = P4::parseP4File(options);

        if (program == nullptr || ::errorCount() > 0) return 1;
        try {
            P4::P4COptionPragmaParser optionsPragmaParser;
            program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

            P4::FrontEnd frontend;
            frontend.addDebugHook(hook);
            program = frontend.run(options, program);
        } catch (const std::exception &bug) {
            std::cerr << bug.what() << std::endl;
            return 1;
        }
        if (program == nullptr || ::errorCount() > 0) return 1;
    } else {
        std::filebuf fb;
        if (fb.open(options.file, std::ios::in) == nullptr) {
            ::error(ErrorType::ERR_IO, "%s: No such file or directory.", options.file);
            return 1;
        }
        std::istream inJson(&fb);
        JSONLoader jsonFileLoader(inJson);
        if (jsonFileLoader.json == nullptr) {
            ::error(ErrorType::ERR_IO, "%s: Not valid input file", options.file);
            return 1;
        }
        program = new IR::P4Program(jsonFileLoader);
        fb.close();
    }

    P4::serializeP4RuntimeIfRequired(program, options);
    if (::errorCount() > 0) return 1;

    BMV2::PsaSwitchMidEnd midEnd(options);
    midEnd.addDebugHook(hook);
    try {
        toplevel = midEnd.process(program);
        if (::errorCount() > 1 || toplevel == nullptr || toplevel->getMain() == nullptr) return 1;
        if (options.dumpJsonFile)
            JSONGenerator(*openFile(options.dumpJsonFile, true), true) << program << std::endl;
    } catch (const std::exception &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0) return 1;

    auto backend =
        new BMV2::PsaSwitchBackend(options, &midEnd.refMap, &midEnd.typeMap, &midEnd.enumMap);

    // Necessary because BMV2Context is expected at the top of stack in further processing
    AutoCompileContext autoContext(new BMV2::BMV2Context(BMV2::PsaSwitchContext::get()));
    try {
        backend->convert(toplevel);
    } catch (const std::exception &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0) return 1;

    if (!options.outputFile.isNullOrEmpty()) {
        std::ostream *out = openFile(options.outputFile, false);
        if (out != nullptr) {
            backend->serialize(*out);
            out->flush();
        }
    }

    return ::errorCount() > 0;
}
