/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#include "backend.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "midend.h"
#include "options.h"
#include "version.h"

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    AutoCompileContext autoTCContext(new TC::TCContext);
    auto &options = TC::TCContext::get().options();
    options.langVersion = TC::TCOptions::FrontendVersion::P4_16;
    options.compilerVersion = version_string();

    if (options.process(argc, argv) != nullptr) {
        options.setInputFile();
    }
    if (::errorCount() > 0) {
        return 1;
    }
    auto hook = options.getDebugHook();
    auto chkprogram = P4::parseP4File(options);
    if (chkprogram == nullptr || ::errorCount() > 0) {
        return 1;
    }

    const IR::P4Program *program = chkprogram;
    if (program == nullptr || ::errorCount() > 0) {
        return 1;
    }
    try {
        P4::P4COptionPragmaParser optionsPragmaParser;
        program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));
        P4::FrontEnd frontend(hook);
        program = frontend.run(options, program);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (program == nullptr || ::errorCount() > 0) {
        return 1;
    }

    P4::serializeP4RuntimeIfRequired(program, options);

    const IR::ToplevelBlock *toplevel = nullptr;
    TC::MidEnd midEnd;
    midEnd.addDebugHook(hook);
    try {
        toplevel = midEnd.run(options, program);
        if (::errorCount() > 1 || toplevel == nullptr) {
            return 1;
        }
        if (toplevel->getMain() == nullptr) {
            ::error("Cannot process input file. Program does not contain a 'main' module");
            return 1;
        }
        if (options.dumpJsonFile)
            JSONGenerator(*openFile(options.dumpJsonFile, true)) << toplevel << std::endl;
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0) {
        return 1;
    }
    TC::Backend backend(toplevel, &midEnd.refMap, &midEnd.typeMap, options);
    if (!backend.process()) return 1;

    if (!options.introspecFile.isNullOrEmpty()) {
        std::ostream *outIntro = openFile(options.introspecFile, false);
        if (outIntro != nullptr) {
            bool serialized = backend.serializeIntrospectionJson(*outIntro);
            if (!serialized) {
                std::remove(options.introspecFile);
                return 1;
            }
        }
    }
    if (::errorCount() > 0) {
        return 1;
    }
    if (!options.outputFile.isNullOrEmpty() || !options.cFile.isNullOrEmpty()) {
        backend.serialize();
    }
    return ::errorCount() > 0;
}
