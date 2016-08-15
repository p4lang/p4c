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

#include <getopt.h>

#include "setup.h"
#include "options.h"
#include "lib/log.h"
#include "lib/exceptions.h"
#include "lib/nullstream.h"
#include "lib/path.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/json_generator.h"

static cstring version = "0.0.4";
extern int verbose;
const char* CompilerOptions::defaultMessage = "Compile a P4 program";

CompilerOptions::CompilerOptions() : Util::Options(defaultMessage) {
    registerOption("-h", nullptr,
                   [this](const char*) { usage(); exit(0); return false; },
                   "Print this help message");
    registerOption("--help", nullptr,
                   [this](const char*) { usage(); exit(0); return false; },
                   "Print this help message");
    registerOption("--version", nullptr,
                   [this](const char*) {
                       std::cerr << binaryName << std::endl;
                       std::cerr << "Version " << version << std::endl;
                       return true; }, "Print compiler version");
    registerOption("-I", "path",
                   [this](const char* arg) {
                       preprocessor_options += std::string(" -I") + arg; return true; },
                   "Specify include path (passed to preprocessor)");
    registerOption("-D", "arg=value",
                   [this](const char* arg) {
                       preprocessor_options += std::string(" -D") + arg; return true; },
                   "Define macro (passed to preprocessor)");
    registerOption("-U", "arg",
                   [this](const char* arg) {
                       preprocessor_options += std::string(" -U") + arg; return true; },
                   "Undefine macro (passed to preprocessor)");
    registerOption("-E", nullptr,
                   [this](const char*) { doNotCompile = true; return true; },
                   "Preprocess only, do not compile (prints program on stdout)");
    registerOption("--p4-14", nullptr,
                   [this](const char*) {
                       langVersion = CompilerOptions::FrontendVersion::P4_14;
                       return true; },
                    "Specify language version to compile");
    registerOption("--p4-16", nullptr,
                   [this](const char*) {
                       langVersion = CompilerOptions::FrontendVersion::P4_16;
                       return true; },
                    "Specify language version to compile");
    registerOption("--p4v", "lang",
                   [this](const char* arg) {
                       if (!strcmp(arg, "1.0") || !strcmp(arg, "14")) {
                           langVersion = CompilerOptions::FrontendVersion::P4_14;
                       } else if (!strcmp(arg, "1.2") || !strcmp(arg, "16")) {
                           langVersion = CompilerOptions::FrontendVersion::P4_16;
                       } else {
                           ::error("Illegal language version %1%", arg);
                           return false;
                       }
                       return true; },
                    "Specify language version to compile");
    registerOption("--target", "target",
                   [this](const char* arg) { target = arg; return true; },
                    "Compile for the specified target");
    registerOption("--pp", "file",
                   [this](const char* arg) { prettyPrintFile = arg; return true; },
                   "Pretty-print the program in the\n"
                   "specified file.");
    registerOption("--toJSON", "file",
                   [this](const char* arg) { dumpJsonFile = arg; return true; },
                   "Dump IR to JSON in the\n"
                   "specified file.");
    registerOption("-o", "outfile",
                   [this](const char* arg) { outputFile = arg; return true; },
                   "Write output to outfile");
    registerOption("-T", "loglevel",
                    [](const char* arg) { ::add_debug_spec(arg); return true; },
                   "[Compiler debugging] Adjust logging level per file (see below)");
    registerOption("-v", nullptr,
                   [this](const char*) { ::verbose++; verbosity++; return true; },
                   "[Compiler debugging] Increase verbosity level (can be repeated)");
    registerOption("--top4", "pass1[,pass2]",
                   [this](const char* arg) {
                       auto copy = strdup(arg);
                       while (auto pass = strsep(&copy, ","))
                           top4.push_back(pass);
                       return true;
                   },
                   "[Compiler debugging] Dump the P4 representation after\n"
                   "passes whose name contains one of `passX' substrings.\n"
                   "When '-v' is used this will include the compiler IR.\n");
    registerOption("--dump", "folder",
                   [this](const char* arg) { dumpFolder = arg; return true; },
                   "[Compiler debugging] Folder where P4 programs are dumped\n");
    registerUsage("loglevel format is:\n"
                  "  sourceFile:level,...,sourceFile:level\n"
                  "where 'sourceFile' is a compiler source file and\n"
                  "'level' is the verbosity level for LOG messages in that file");
}

void CompilerOptions::setInputFile() {
    if (remainingOptions.size() > 1) {
        ::error("Only one input file must be specified: %s",
                cstring::join(remainingOptions.begin(), remainingOptions.end(), ","));
        usage();
    } else if (remainingOptions.size() == 0) {
        ::error("No input files specified");
        usage();
    } else {
        file = remainingOptions.at(0);
    }
}

FILE* CompilerOptions::preprocess() {
    FILE* in = nullptr;

    if (file == "-") {
        file = "<stdin>";
        in = stdin;
    } else {
#ifdef __clang__
        /* FIXME -- while clang has a 'cpp' executable, its broken and doesn't work right, so
         * we need to run clang -E instead.  This should be managed by autoconf (figure out how
         * to portably run the c preprocessor) */
        std::string cmd("cc -E -x c");
#else
        std::string cmd("cpp");
#endif
        cmd += cstring(" -undef -nostdinc -I") +
                p4includePath + " " + preprocessor_options + " " + file;
        if (verbose)
            std::cerr << "Invoking preprocessor " << std::endl << cmd << std::endl;
        in = popen(cmd.c_str(), "r");
        if (in == nullptr) {
            ::error("Error invoking preprocessor");
            perror("");
            return nullptr;
        }
        close_input = true;
    }

    if (doNotCompile) {
        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&line, &len, in)) != -1)
            printf("%s", line);
        closeInput(in);
        return nullptr;
    }
    return in;
}

void CompilerOptions::closeInput(FILE* inputStream) const {
    if (close_input) {
        int exitCode = pclose(inputStream);
        if (exitCode != 0)
            ::error("Preprocessor returned exit code %d; aborting compilation", exitCode);
    }
}

// From (folder, file.ext, suffix)  returns
// folder/file-suffix.ext
static cstring makeFileName(cstring folder, cstring name, cstring baseSuffix) {
    Util::PathName filename(name);
    Util::PathName newName(filename.getBasename() + baseSuffix + "." + filename.getExtension());
    auto result = Util::PathName(folder).join(newName.toString());
    return result.toString();
}

bool CompilerOptions::isv1() const {
    return langVersion == CompilerOptions::FrontendVersion::P4_14;
}

void CompilerOptions::dumpPass(const char* manager, unsigned seq, const char* pass,
                               const IR::Node* node) const {
    // Some pass names are currently C++ class names mangled;
    // this is a weak attempt at making them more readable.
    bool verbose = verbosity > 0;

    std::string p = pass;
    size_t last = p.find_last_of("0123456789", strlen(pass) - 3);
    if (last != strlen(pass))
        pass = pass + last + 1;
    cstring name = cstring(manager) + "_" + Util::toString(seq) + "_" + pass;
    if (verbose)
        std::cerr << name << std::endl;



    for (auto s : top4) {
        if (strstr(name.c_str(), s.c_str()) != nullptr) {
            cstring suffix = cstring("-") + name;
            cstring filename = file;
            if (filename == "-")
                filename = "tmp.p4";

            cstring fileName = makeFileName(dumpFolder, filename, suffix);
            auto stream = openFile(fileName, true);
            if (stream != nullptr) {
                if (verbose)
                    std::cerr << "Writing program to " << fileName << std::endl;
                P4::ToP4 toP4(stream, verbose);
                node->apply(toP4);
            }
        }
    }
    if (dumpJsonFile != "") {
        auto stream = openFile(dumpJsonFile, true);
        std::cout << "Json File: " << dumpJsonFile << std::endl;
        if (stream != nullptr) {
            JSONGenerator json(*stream);
            json << node << std::endl;
        }
    }
}

DebugHook CompilerOptions::getDebugHook() const {
    auto dp = std::bind(&CompilerOptions::dumpPass, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4);
    return dp;
}
