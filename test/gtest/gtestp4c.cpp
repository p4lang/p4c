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

#include "gtest/gtest.h"
#include "helpers.h"
#include "lib/log.h"
#include "lib/options.h"

class GTestOptions : public Util::Options {
 public:
    GTestOptions() : Util::Options("Additional p4c-specific options:") {
        registerOption("--help", nullptr,
                       [this](const char*) {
                           std::cerr << std::endl;
                           usage();
                           exit(0);
                           return false;
                       }, "Print this help message");
        registerOption("-v", nullptr,
                       [this](const char*) { Log::increaseVerbosity(); return true; },
                       "[Compiler debugging] Increase verbosity level (can be repeated)");
        registerOption("-T", "loglevel",
                       [](const char* arg) { Log::addDebugSpec(arg); return true; },
                       "Adjust logging level per file (see below)");
        registerUsage("loglevel format is:\n"
                      "  sourceFile:level,...,sourceFile:level\n"
                      "where 'sourceFile' is a compiler source file and\n"
                      "'level' is the verbosity level for LOG messages in that file");
    }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    GTestOptions options;
    options.process(argc, argv);
    if (::ErrorReporter::instance.getDiagnosticCount() > 0) return -1;

    // Initialize the global test environment.
    (void) P4CTestEnvironment::get();

    return RUN_ALL_TESTS();
}
