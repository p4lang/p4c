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

#ifndef FRONTENDS_COMMON_PARSER_OPTIONS_H_
#define FRONTENDS_COMMON_PARSER_OPTIONS_H_
#include <set>
#include "lib/options.h"
#include "ir/configuration.h"


// Base class for parser options.
// This class contains the options for the parser.

class ParserOptions : public Util::Options {
    static const char* defaultMessage;

 public:
    ParserOptions();


    enum class FrontendVersion {
        P4_14,
        P4_16
    };

    // Target
    cstring target = nullptr;
    // Architecture
    cstring arch = nullptr;
    // Compiler version.
    cstring compilerVersion;
    // options to pass to preprocessor
    cstring preprocessor_options = "";

    // if true preprocess only
    bool doNotCompile = false;
    // if true skip preprocess
    bool doNotPreprocess = false;
    // Dump and undump the IR tree
    bool debugJson = false;
    // substrings matched agains pass names
    std::vector<cstring> top4;
    // debugging dumps of programs written in this folder
    cstring dumpFolder = ".";
    // if this flag is true, compile program in non-debug mode
    bool ndebug = false;
    // Pretty-print the program in the specified file
    cstring prettyPrintFile = nullptr;
    // Dump a JSON representation of the IR in the file
    cstring dumpJsonFile = nullptr;
    // annotation names that are to be ignored by the compiler
    std::set<cstring> disabledAnnotations;


};
#endif /* FRONTENDS_COMMON_PARSER_OPTIONS_H_ */
