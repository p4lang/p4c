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

/* -*-C++-*- */

#ifndef FRONTENDS_COMMON_OPTIONS_H_
#define FRONTENDS_COMMON_OPTIONS_H_

#include <filesystem>

#include "parser_options.h"
// for p4::P4RuntimeFormat definition
#include "control-plane/p4RuntimeTypes.h"

namespace P4C {

class CompilerOptions : public ParserOptions {
 protected:
    bool validateOptions() const override;

 public:
    explicit CompilerOptions(std::string_view defaultMessage = "Compile a P4 program");

    // If true, skip frontend passes whose names are contained in
    // passesToExcludeFrontend vector.
    bool excludeFrontendPasses = false;
    bool listFrontendPasses = false;
    // If true, skip midend passes whose names are contained in
    // passesToExcludeMidend vector.
    bool excludeMidendPasses = false;
    bool listMidendPasses = false;
    // If true, skip backend passes whose names are contained in
    // passesToExcludeBackend vector.
    bool excludeBackendPasses = false;
    // Strings matched against pass names that should be excluded from frontend
    // passes.
    std::vector<cstring> passesToExcludeFrontend;
    // Strings matched against pass names that should be excluded from midend
    // passes.
    std::vector<cstring> passesToExcludeMidend;
    // Strings matched against pass names that should be excluded from backend
    // passes.
    std::vector<cstring> passesToExcludeBackend;
    // Dump a JSON representation of the IR in the file.
    std::filesystem::path dumpJsonFile;
    // Dump and undump the IR tree.
    bool debugJson = false;
    // if this flag is true, compile program in non-debug mode.
    bool ndebug = false;
    // Write a P4Runtime control plane API description to the specified file.
    cstring p4RuntimeFile = nullptr;
    // Write static table entries as a P4Runtime WriteRequest message to the
    // specified file.
    cstring p4RuntimeEntriesFile = nullptr;
    // Write P4Runtime control plane API description to the specified files.
    cstring p4RuntimeFiles = nullptr;
    // Write static table entries as a P4Runtime WriteRequest message to the
    // specified files.
    cstring p4RuntimeEntriesFiles = nullptr;
    // Choose format for P4Runtime API description.
    P4::P4RuntimeFormat p4RuntimeFormat = P4::P4RuntimeFormat::BINARY;
    // Pretty-print the program in the specified file.
    std::filesystem::path prettyPrintFile;
    // Target.
    cstring target = nullptr;
    // Architecture.
    cstring arch = nullptr;
    // If true, unroll all parser loops inside the midend.
    bool loopsUnrolling = false;

    // General optimization options -- can be interpreted by backends in various ways
    int optimizationLevel = 1;
    bool optimizeDebug = false;  // optimize favoring debuggability
    bool optimizeSize = false;   // optimize favoring size

    virtual bool enable_intrinsic_metadata_fix();
};

}  // namespace P4C

#endif /* FRONTENDS_COMMON_OPTIONS_H_ */
