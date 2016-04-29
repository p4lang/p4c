#include <stdio.h>
#include <string>
#include <iostream>
#include <regex>

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
#include "options.h"
#include "midend.h"

class PrintDump {
    V12Test::V12TestOptions& options;
    std::regex* regex = nullptr;
    bool verbose;

 public:
    explicit PrintDump(V12Test::V12TestOptions& options) :
            options(options), verbose(options.verbosity > 0) {
        if (options.top4 != nullptr) {
            cstring r = cstring(".*") + options.top4 + ".*";
            regex = new std::regex(r.c_str());
        }
    }
    void printPass(const char* manager, unsigned seq, const char* pass, const IR::Node* node) {
        // Pass names are currently C++ class names mangled; this is a weak attempt at making them
        // more readable.
        std::string p = pass;
        size_t last = p.find_last_of("0123456789", strlen(pass) - 3);
        if (last != strlen(pass))
            pass = pass + last + 1;
        cstring name = cstring(manager) + "_" + Util::toString(seq) + "_" + pass;
        std::cerr << name << std::endl;

        if (regex != nullptr) {
            bool match = std::regex_match(name.c_str(), *regex);
            if (match) {
                auto stream = options.dumpStream(name);
                if (verbose)
                    std::cerr << "Writing program to " << options.dumpFileName(name) << std::endl;
                P4::ToP4 toP4(stream, options.file);
                node->apply(toP4);
            }
        }
    }
};


int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    V12Test::V12TestOptions options;
    options.langVersion = CompilerOptions::FrontendVersion::P4v1_2;

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    PrintDump pd(options);
    auto dp = std::bind(&PrintDump::printPass, &pd,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4);
    auto program = parseP4File(options);
    if (program != nullptr && ::errorCount() == 0) {
        FrontEnd fe;
        if (::verbose)
            fe.addDebugHook(dp);
        program = fe.run(options, program);
        if (program != nullptr && ::errorCount() == 0) {
            V12Test::MidEnd midEnd;
            if (::verbose)
                midEnd.addDebugHook(dp);
            (void)midEnd.process(options, program);
        }
    }
    if (verbose)
        std::cerr << "Done." << std::endl;
    return ::errorCount() > 0;
}
