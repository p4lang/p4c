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

#include "options.h"

#include <stdlib.h>
#include <string.h>

#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "lib/error.h"
#include "lib/error_catalog.h"

CompilerOptions::CompilerOptions() : ParserOptions() {
    registerOption(
        "--excludeFrontendPasses", "pass1[,pass2]",
        [this](const char *arg) {
            excludeFrontendPasses = true;
            auto copy = strdup(arg);
            while (auto pass = strsep(&copy, ",")) passesToExcludeFrontend.push_back(pass);
            return true;
        },
        "Exclude passes from frontend passes whose name is equal\n"
        "to one of `passX' strings.\n");
    registerOption(
        "--listFrontendPasses", nullptr,
        [this](const char *) {
            listFrontendPasses = true;
            P4::FrontEnd frontend;
            frontend.run(*this, nullptr, false, outStream);
            exit(0);
            return false;
        },
        "List exact names of all frontend passes\n");
    registerOption(
        "--excludeMidendPasses", "pass1[,pass2]",
        [this](const char *arg) {
            excludeMidendPasses = true;
            auto copy = strdup(arg);
            while (auto pass = strsep(&copy, ",")) passesToExcludeMidend.push_back(pass);
            return true;
        },
        "Exclude passes from midend passes whose name is equal\n"
        "to one of `passX' strings.\n");
    registerOption(
        "--toJSON", "file",
        [this](const char *arg) {
            dumpJsonFile = arg;
            return true;
        },
        "Dump the compiler IR after the midend as JSON in the specified file.");
    registerOption(
        "--ndebug", nullptr,
        [this](const char *) {
            ndebug = true;
            return true;
        },
        "Compile program in non-debug mode.\n");
    registerOption(
        "--testJson", nullptr,
        [this](const char *) {
            debugJson = true;
            return true;
        },
        "[Compiler debugging] Dump and undump the IR");
    registerOption(
        "--pp", "file",
        [this](const char *arg) {
            prettyPrintFile = arg;
            return true;
        },
        "Pretty-print the program in the specified file.");
    registerOption(
        "--p4runtime-file", "file",
        [this](const char *arg) {
            p4RuntimeFile = arg;
            return true;
        },
        "Write a P4Runtime control plane API description to the specified "
        "file.\n"
        "[Deprecated; use '--p4runtime-files' instead].");
    registerOption(
        "--p4runtime-entries-file", "file",
        [this](const char *arg) {
            p4RuntimeEntriesFile = arg;
            return true;
        },
        "Write static table entries as a P4Runtime WriteRequest message"
        "to the specified file.\n"
        "[Deprecated; use '--p4runtime-entries-files' instead].");
    registerOption(
        "--p4runtime-files", "filelist",
        [this](const char *arg) {
            p4RuntimeFiles = arg;
            return true;
        },
        "Write the P4Runtime control plane API description to the specified\n"
        "files (comma-separated list). The format is inferred from the file\n"
        "suffix: .txt, .json, .bin");
    registerOption(
        "--p4runtime-entries-files", "files",
        [this](const char *arg) {
            p4RuntimeEntriesFiles = arg;
            return true;
        },
        "Write static table entries as a P4Runtime WriteRequest message\n"
        "to the specified files (comma-separated list); the file format is\n"
        "inferred from the suffix. Legal suffixes are .json, .txt and .bin");
    registerOption(
        "--p4runtime-format", "{binary,json,text}",
        [this](const char *arg) {
            if (!strcmp(arg, "binary")) {
                p4RuntimeFormat = P4::P4RuntimeFormat::BINARY;
            } else if (!strcmp(arg, "json")) {
                p4RuntimeFormat = P4::P4RuntimeFormat::JSON;
            } else if (!strcmp(arg, "text")) {
                p4RuntimeFormat = P4::P4RuntimeFormat::TEXT;
            } else {
                ::error(ErrorType::ERR_INVALID, "Illegal P4Runtime format %1%", arg);
                return false;
            }
            return true;
        },
        "Choose output format for the P4Runtime API description (default is "
        "binary).\n"
        "[Deprecated; use '--p4runtime-files' instead].");
    registerOption(
        "--target", "target",
        [this](const char *arg) {
            target = arg;
            return true;
        },
        "Compile for the specified target device.");
    registerOption(
        "--arch", "arch",
        [this](const char *arg) {
            arch = arg;
            return true;
        },
        "Compile for the specified architecture.");
    registerOption(
        "--loopsUnroll", nullptr,
        [this](const char *) {
            loopsUnrolling = true;
            return true;
        },
        "Unrolling all parser's loops");
}

bool CompilerOptions::enable_intrinsic_metadata_fix() { return true; }

void CompilerOptions::validateOptions() const {
    if (!p4RuntimeFile.isNullOrEmpty()) {
        ::warning(ErrorType::WARN_DEPRECATED,
                  "'--p4runtime-file' and '--p4runtime-format' are deprecated, "
                  "consider using '--p4runtime-files' instead");
    }
    if (!p4RuntimeEntriesFile.isNullOrEmpty()) {
        ::warning(ErrorType::WARN_DEPRECATED,
                  "'--p4runtime-entries-file' is deprecated, "
                  "consider using '--p4runtime-entries-files' instead");
    }
}
