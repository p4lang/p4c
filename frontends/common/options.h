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

#include <regex>
#include "lib/cstring.h"
#include "lib/options.h"
#include "ir/ir.h"  // for DebugHook definition
// for p4::P4RuntimeFormat definition
#include "control-plane/p4RuntimeSerializer.h"

// Base class for compiler options.
// This class contains the options for the front-ends.
// Each back-end should subclass this file.
class CompilerOptions : public Util::Options {
    bool close_input = false;
    static const char* defaultMessage;

 protected:
    // Function that is returned by getDebugHook.
    void dumpPass(const char* manager, unsigned seq, const char* pass, const IR::Node* node) const;

 public:
    CompilerOptions();
    std::vector<const char*>* process(int argc, char* const argv[]);

    enum class FrontendVersion {
        P4_14,
        P4_16
    };

    // Name of executable that is being run.
    cstring exe_name;
    // Which language to compile
    FrontendVersion langVersion = FrontendVersion::P4_14;
    // options to pass to preprocessor
    cstring preprocessor_options = "";
    // file to compile (- for stdin)
    cstring file = nullptr;
    // if true preprocess only
    bool doNotCompile = false;
    // if true skip preprocess
    bool doNotPreprocess = false;
    // debugging dumps of programs written in this folder
    cstring dumpFolder = ".";
    // Pretty-print the program in the specified file
    cstring prettyPrintFile = nullptr;
    // file to output to
    cstring outputFile = nullptr;
    // Compiler version.
    cstring compilerVersion;

    // Dump a JSON representation of the IR in the file
    cstring dumpJsonFile = nullptr;

    // Dump and undump the IR tree
    bool debugJson = false;

    // Write a P4Runtime control plane API description to the specified file.
    cstring p4RuntimeFile = nullptr;

    // Choose format for P4Runtime API description.
    P4::P4RuntimeFormat p4RuntimeFormat = P4::P4RuntimeFormat::BINARY;

    // Compiler target architecture
    cstring target = nullptr;
    // substrings matched agains pass names
    std::vector<cstring> top4;

    // Expect that the only remaining argument is the input file.
    void setInputFile();

    // Returns the output of the preprocessor.
    FILE* preprocess();
    // Closes the input stream returned by preprocess.
    void closeInput(FILE* input) const;

    // True if we are compiling a P4 v1.0 or v1.1 program
    bool isv1() const;
    // Get a debug hook function suitable for insertion
    // in the pass managers that are executed.
    DebugHook getDebugHook() const;
};

#endif /* FRONTENDS_COMMON_OPTIONS_H_ */
