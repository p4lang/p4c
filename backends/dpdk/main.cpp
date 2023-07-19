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

#include <cstdio>
#include <fstream>  // IWYU pragma: keep
#include <iostream>
#include <string>

#include "backends/dpdk/backend.h"
#include "backends/dpdk/control-plane/bfruntime_arch_handler.h"
#include "backends/dpdk/midend.h"
#include "backends/dpdk/options.h"
#include "backends/dpdk/tdiConf.h"
#include "backends/dpdk/version.h"
#include "control-plane/bfruntime_ext.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/exename.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"

void generateTDIBfrtJson(bool isTDI, const IR::P4Program *program, DPDK::DpdkOptions &options) {
    auto p4RuntimeSerializer = P4::P4RuntimeSerializer::get();
    if (options.arch == "psa")
        p4RuntimeSerializer->registerArch(
            "psa", new P4::ControlPlaneAPI::Standard::PSAArchHandlerBuilderForDPDK());
    if (options.arch == "pna")
        p4RuntimeSerializer->registerArch(
            "pna", new P4::ControlPlaneAPI::Standard::PNAArchHandlerBuilderForDPDK());
    auto p4Runtime = P4::generateP4Runtime(program, options.arch);

    cstring filename = isTDI ? options.tdiFile : options.bfRtSchema;
    auto p4rt = new P4::BFRT::BFRuntimeSchemaGenerator(*p4Runtime.p4Info, isTDI, options);
    std::ostream *out = openFile(filename, false);
    if (!out) {
        ::error(ErrorType::ERR_IO, "Could not open file: %1%", filename);
        return;
    }
    p4rt->serializeBFRuntimeSchema(out);
}

int main(int argc, char *const argv[]) {
    setup_gc_logging();

    AutoCompileContext autoDpdkContext(new DPDK::DpdkContext);
    auto &options = DPDK::DpdkContext::get().options();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = DPDK_VERSION_STRING;

    if (options.process(argc, argv) != nullptr) {
        if (options.loadIRFromJson == false) options.setInputFile();
    }
    if (::errorCount() > 0) return 1;

    auto hook = options.getDebugHook();

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
            ::error(ErrorType::ERR_NOT_FOUND, "%s: No such file or directory.", options.file);
            return 1;
        }
        std::istream inJson(&fb);
        JSONLoader jsonFileLoader(inJson);
        if (jsonFileLoader.json == nullptr) {
            ::error(ErrorType::ERR_INVALID, "Not valid input file");
            return 1;
        }
        program = new IR::P4Program(jsonFileLoader);
        fb.close();
    }

    P4::serializeP4RuntimeIfRequired(program, options);
    if (::errorCount() > 0) return 1;

    if (!options.tdiBuilderConf.isNullOrEmpty()) {
        DPDK::TdiBfrtConf::generate(program, options);
    }

    if (!options.bfRtSchema.isNullOrEmpty()) {
        generateTDIBfrtJson(false, program, options);
    }
    if (!options.tdiFile.isNullOrEmpty()) {
        generateTDIBfrtJson(true, program, options);
    }

    if (::errorCount() > 0) return 1;
    auto p4info = *P4::generateP4Runtime(program, options.arch).p4Info;
    DPDK::DpdkMidEnd midEnd(options);
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

    auto backend = new DPDK::DpdkBackend(options, &midEnd.refMap, &midEnd.typeMap, p4info);

    backend->convert(toplevel);
    if (::errorCount() > 0) return 1;

    if (!options.outputFile.isNullOrEmpty()) {
        std::ostream *out = openFile(options.outputFile, false);
        if (out != nullptr) {
            backend->codegen(*out);
            out->flush();
        }
    }

    return ::errorCount() > 0;
}
