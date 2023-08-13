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

#include <stdlib.h>
#include <string.h>

#include <exception>
#include <fstream>  // IWYU pragma: keep
#include <iomanip>
#include <iostream>
#include <string>

#include "backends/p4test/version.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "ir/dump.h"
#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"
#include "ir/node.h"
#include "lib/compile_context.h"
#include "lib/crash.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "midend.h"

class P4TestOptions : public CompilerOptions {
 public:
    bool parseOnly = false;
    bool validateOnly = false;
    bool loadIRFromJson = false;
    P4TestOptions() {
        registerOption(
            "--listMidendPasses", nullptr,
            [this](const char *) {
                listMidendPasses = true;
                loadIRFromJson = false;
                P4Test::MidEnd MidEnd(*this, outStream);
                exit(0);
                return false;
            },
            "[p4test] Lists exact name of all midend passes.\n");
        registerOption(
            "--parse-only", nullptr,
            [this](const char *) {
                parseOnly = true;
                return true;
            },
            "only parse the P4 input, without any further processing");
        registerOption(
            "--validate", nullptr,
            [this](const char *) {
                validateOnly = true;
                return true;
            },
            "Validate the P4 input, running just the front-end");
        registerOption(
            "--fromJSON", "file",
            [this](const char *arg) {
                loadIRFromJson = true;
                file = arg;
                return true;
            },
            "read previously dumped json instead of P4 source code");
        registerOption(
            "--turn-off-logn", nullptr,
            [](const char *) {
                ::Log::Detail::enableLoggingGlobally = false;
                return true;
            },
            "Turn off LOGN() statements in the compiler.\n"
            "Use '@__debug' annotation to enable LOGN on "
            "the annotated P4 object within the source code.\n");
    }
};

using P4TestContext = P4CContextWithOptions<P4TestOptions>;

static void log_dump(const IR::Node *node, const char *head) {
    if (node && LOGGING(1)) {
        if (head)
            std::cout << '+' << std::setw(strlen(head) + 6) << std::setfill('-') << "+\n| " << head
                      << " |\n"
                      << '+' << std::setw(strlen(head) + 3) << "+" << std::endl
                      << std::setfill(' ');
        if (LOGGING(2))
            dump(node);
        else
            std::cout << *node << std::endl;
    }
}

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    AutoCompileContext autoP4TestContext(new P4TestContext);
    auto &options = P4TestContext::get().options();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = P4TEST_VERSION_STRING;

    if (options.process(argc, argv) != nullptr) {
        if (options.loadIRFromJson == false) options.setInputFile();
    }
    if (::errorCount() > 0) return 1;
    const IR::P4Program *program = nullptr;
    auto hook = options.getDebugHook();
    if (options.loadIRFromJson) {
        std::ifstream json(options.file);
        if (json) {
            JSONLoader loader(json);
            const IR::Node *node = nullptr;
            loader >> node;
            if (!(program = node->to<IR::P4Program>()))
                error(ErrorType::ERR_INVALID, "%s is not a P4Program in json format", options.file);
        } else {
            error(ErrorType::ERR_IO, "Can't open %s", options.file);
        }
    } else {
        program = P4::parseP4File(options);

        if (program != nullptr && ::errorCount() == 0) {
            P4::P4COptionPragmaParser optionsPragmaParser;
            program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

            if (!options.parseOnly) {
                try {
                    P4::FrontEnd fe;
                    fe.addDebugHook(hook);
                    program = fe.run(options, program);
                } catch (const std::exception &bug) {
                    std::cerr << bug.what() << std::endl;
                    return 1;
                }
            }
        }
    }

    log_dump(program, "Initial program");
    if (program != nullptr && ::errorCount() == 0) {
        P4::serializeP4RuntimeIfRequired(program, options);

        if (!options.parseOnly && !options.validateOnly) {
            P4Test::MidEnd midEnd(options);
            midEnd.addDebugHook(hook);
#if 0
            /* doing this breaks the output until we get dump/undump of srcInfo */
            if (options.debugJson) {
                std::stringstream tmp;
                JSONGenerator gen(tmp);
                gen << program;
                JSONLoader loader(tmp);
                loader >> program;
            }
#endif
            const IR::ToplevelBlock *top = nullptr;
            try {
                top = midEnd.process(program);
                // This can modify program!
                log_dump(program, "After midend");
                log_dump(top, "Top level block");
            } catch (const std::exception &bug) {
                std::cerr << bug.what() << std::endl;
                return 1;
            }
        }
        if (program) {
            if (options.dumpJsonFile)
                JSONGenerator(*openFile(options.dumpJsonFile, true), true) << program << std::endl;
            if (options.debugJson) {
                std::stringstream ss1, ss2;
                JSONGenerator gen1(ss1), gen2(ss2);
                gen1 << program;

                const IR::Node *node = nullptr;
                JSONLoader loader(ss1);
                loader >> node;

                gen2 << node;
                if (ss1.str() != ss2.str()) {
                    error(ErrorType::ERR_UNEXPECTED, "json mismatch");
                    std::ofstream t1("t1.json"), t2("t2.json");
                    t1 << ss1.str() << std::flush;
                    t2 << ss2.str() << std::flush;
                    auto rv = system("json_diff t1.json t2.json");
                    if (rv != 0)
                        ::warning(ErrorType::WARN_FAILED, "json_diff failed with code %1%", rv);
                }
            }
        }
    }

    if (Log::verbose()) std::cerr << "Done." << std::endl;
    return ::errorCount() > 0;
}
