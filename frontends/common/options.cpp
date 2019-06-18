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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

#include "options.h"
#include "lib/log.h"
#include "lib/exceptions.h"
#include "lib/nullstream.h"
#include "lib/path.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/json_generator.h"

const char* p4includePath = CONFIG_PKGDATADIR "/p4include";
const char* p4_14includePath = CONFIG_PKGDATADIR "/p4_14include";

const char* CompilerOptions::defaultMessage = "Compile a P4 program";

CompilerOptions::CompilerOptions() : Util::Options(defaultMessage) {
    registerOption("--help", nullptr,
                   [this](const char*) { usage(); exit(0); return false; },
                   "Print this help message");
    registerOption("--version", nullptr,
                   [this](const char*) {
                       std::cerr << binaryName << std::endl;
                       std::cerr << "Version " << compilerVersion << std::endl;
                       exit(0); return false; }, "Print compiler version");
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
    registerOption("--nocpp", nullptr,
                   [this](const char*) { doNotPreprocess = true; return true; },
                   "Skip preprocess, assume input file is already preprocessed.");
    registerOption("--p4v", "{14|16}",
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
                    "[Deprecated; use --std instead] Specify language version to compile.",
                    OptionFlags::Hide);
    registerOption("--std", "{p4-14|p4-16}",
                   [this](const char* arg) {
                       if (!strcmp(arg, "14") || !strcmp(arg, "p4-14")) {
                           langVersion = CompilerOptions::FrontendVersion::P4_14;
                       } else if (!strcmp(arg, "16") || !strcmp(arg, "p4-16")) {
                           langVersion = CompilerOptions::FrontendVersion::P4_16;
                       } else {
                           ::error("Illegal language version %1%", arg);
                           return false;
                       }
                       return true; },
                    "Specify language version to compile.");
    registerOption("--target", "target",
                   [this](const char* arg) { target = arg; return true; },
                    "Compile for the specified target device.");
    registerOption("--arch", "arch",
                   [this](const char* arg) { arch = arg; return true; },
                   "Compile for the specified architecture.");
    registerOption("--pp", "file",
                   [this](const char* arg) { prettyPrintFile = arg; return true; },
                   "Pretty-print the program in the specified file.");
    registerOption("--toJSON", "file",
                   [this](const char* arg) { dumpJsonFile = arg; return true; },
                   "Dump the compiler IR after the midend as JSON in the specified file.");
    registerOption("--p4runtime-files", "filelist",
                   [this](const char* arg) { p4RuntimeFiles = arg; return true; },
                   "Write the P4Runtime control plane API description to the specified\n"
                   "files (comma-separated list). The format is inferred from the file\n"
                   "suffix: .txt, .json, .bin");
    registerOption("--p4runtime-file", "file",
                   [this](const char* arg) { p4RuntimeFile = arg; return true; },
                   "Write a P4Runtime control plane API description to the specified file.\n"
                   "[Deprecated; use '--p4runtime-files' instead].");
    registerOption("--p4runtime-entries-file", "file",
                   [this](const char* arg) { p4RuntimeEntriesFile = arg; return true; },
                   "Write static table entries as a P4Runtime WriteRequest message"
                   "to the specified file.\n"
                   "[Deprecated; use '--p4runtime-entries-files' instead].");
    registerOption("--p4runtime-entries-files", "files",
                   [this](const char* arg) { p4RuntimeEntriesFiles = arg; return true; },
                   "Write static table entries as a P4Runtime WriteRequest message\n"
                   "to the specified files (comma-separated list); the file format is\n"
                   "inferred from the suffix. Legal suffixes are .json, .txt and .bin");
    registerOption("--p4runtime-format", "{binary,json,text}",
                   [this](const char* arg) {
                       if (!strcmp(arg, "binary")) {
                           p4RuntimeFormat = P4::P4RuntimeFormat::BINARY;
                       } else if (!strcmp(arg, "json")) {
                           p4RuntimeFormat = P4::P4RuntimeFormat::JSON;
                       } else if (!strcmp(arg, "text")) {
                           p4RuntimeFormat = P4::P4RuntimeFormat::TEXT;
                       } else {
                           ::error("Illegal P4Runtime format %1%", arg);
                           return false;
                       }
                       return true; },
                   "Choose output format for the P4Runtime API description (default is binary).\n"
                   "[Deprecated; use '--p4runtime-files' instead].");
    registerOption("--Wdisable", "diagnostic",
        [](const char *diagnostic) {
            if (diagnostic) {
                P4CContext::get().setDiagnosticAction(diagnostic,
                                                      DiagnosticAction::Ignore);
            } else {
                auto action = DiagnosticAction::Ignore;
                P4CContext::get().setDefaultWarningDiagnosticAction(action);
            }
            return true;
        }, "Disable a compiler diagnostic, or disable all warnings if no "
           "diagnostic is specified.",
        OptionFlags::OptionalArgument);
    registerOption("--Wwarn", "diagnostic",
        [](const char *diagnostic) {
            if (diagnostic) {
                P4CContext::get().setDiagnosticAction(diagnostic,
                                                      DiagnosticAction::Warn);
            } else {
                auto action = DiagnosticAction::Warn;
                P4CContext::get().setDefaultWarningDiagnosticAction(action);
            }
            return true;
        }, "Report a warning for a compiler diagnostic, or treat all warnings "
           "as warnings (the default) if no diagnostic is specified.",
        OptionFlags::OptionalArgument);
    registerOption("--Werror", "diagnostic",
        [](const char *diagnostic) {
            if (diagnostic) {
                P4CContext::get().setDiagnosticAction(diagnostic,
                                                      DiagnosticAction::Error);
            } else {
                auto action = DiagnosticAction::Error;
                P4CContext::get().setDefaultWarningDiagnosticAction(action);
            }
            return true;
        }, "Report an error for a compiler diagnostic, or treat all warnings as "
           "errors if no diagnostic is specified.",
        OptionFlags::OptionalArgument);
    registerOption("--maxErrorCount", "errorCount",
                   [](const char *arg) {
                       auto maxError = strtoul(arg, nullptr, 10);
                       P4CContext::get().errorReporter().setMaxErrorCount(maxError);
                       return true; },
                   "Set the maximum number of errors to display before failing.");
    registerOption("--testJson", nullptr,
                    [this](const char*) { debugJson = true; return true; },
                    "[Compiler debugging] Dump and undump the IR");
    registerOption("-T", "loglevel",
                   [](const char* arg) { Log::addDebugSpec(arg); return true; },
                   "[Compiler debugging] Adjust logging level per file (see below)");
    registerOption("-v", nullptr,
                   [](const char*) { Log::increaseVerbosity(); return true; },
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
    registerOption("--ndebug", nullptr,
                   [this](const char*) { ndebug = true; return true; },
                  "Compile program in non-debug mode.\n");
}

void CompilerOptions::setInputFile() {
    if (remainingOptions.size() > 1) {
        ::error("Only one input file must be specified: %s",
                cstring::join(remainingOptions.begin(), remainingOptions.end(), ","));
        usage();
        exit(1);
    } else if (remainingOptions.size() == 0) {
        ::error("No input files specified");
        usage();
        exit(1);
    } else {
        file = remainingOptions.at(0);
    }
}

namespace {

template <size_t N>
static void convertToAbsPath(const char* const relPath, char (&output)[N]) {
    output[0] = '\0';  // Default to the empty string, indicating failure.

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return;
    const size_t cwdLen = strlen(cwd);
    if (cwdLen == 0) return;
    const char* separator = cwd[cwdLen - 1] == '/' ? "" : "/";

    // Construct an absolute path. We're assuming that @relPath is relative to
    // the current working directory.
    snprintf(output, N, "%s%s%s", cwd, separator, relPath);
}

bool setIncludePathIfExists(const char*& includePathOut,
                            const char* possiblePath) {
    struct stat st;
    char buffer[PATH_MAX];
    int len;
    if ((len = readlink(possiblePath, buffer, sizeof(buffer))) > 0) {
        buffer[len] = 0;
        possiblePath = buffer; }
    if (!(stat(possiblePath, &st) >= 0 && S_ISDIR(st.st_mode))) return false;
    includePathOut = strdup(possiblePath);
    return true;
}

}  // namespace

std::vector<const char*>* CompilerOptions::process(int argc, char* const argv[]) {
    char buffer[PATH_MAX];
    int len;
    /* find the path of the executable.  We use a number of techniques that may fail
     * or work on different systems, and take the first working one we find.  Fall
     * back to not overriding the compiled-in installation path */
    if ((len = readlink("/proc/self/exe", buffer, sizeof(buffer))) > 0 ||
        (len = readlink("/proc/curproc/exe", buffer, sizeof(buffer))) > 0 ||
        (len = readlink("/proc/curproc/file", buffer, sizeof(buffer))) > 0 ||
        (len = readlink("/proc/self/path/a.out", buffer, sizeof(buffer))) > 0) {
        buffer[len] = 0;
    } else if (argv[0][0] == '/') {
        snprintf(buffer, sizeof(buffer), "%s", argv[0]);
    } else if (strchr(argv[0], '/')) {
        convertToAbsPath(argv[0], buffer);
    } else if (getenv("_")) {
        strncpy(buffer, getenv("_"), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = 0;
    } else {
        buffer[0] = 0; }

    if (char *p = strrchr(buffer, '/')) {
        ++p;
        exe_name = p;
        snprintf(p, buffer + sizeof(buffer) - p, "p4include");
        if (!setIncludePathIfExists(p4includePath, buffer)) {
            snprintf(p, buffer + sizeof(buffer) - p, "../p4include");
            setIncludePathIfExists(p4includePath, buffer);
        }
        snprintf(p, buffer + sizeof(buffer) - p, "p4_14include");
        if (!setIncludePathIfExists(p4_14includePath, buffer)) {
            snprintf(p, buffer + sizeof(buffer) - p, "../p4_14include");
            setIncludePathIfExists(p4_14includePath, buffer);
        }
    }

    auto remainingOptions = Util::Options::process(argc, argv);
    validateOptions();
    return remainingOptions;
}

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

FILE* CompilerOptions::preprocess() {
    FILE* in = nullptr;

    if (file == "-") {
        file = "<stdin>";
        in = stdin;
    } else {
#ifdef __clang__
        std::string cmd("cc -E -x c -Wno-comment");
#else
        std::string cmd("cpp");
#endif
        // the p4c driver sets environment variables for include
        // paths.  check the environment and add these to the command
        // line for the preprocessor
        char * driverP4IncludePath =
          isv1() ? getenv("P4C_14_INCLUDE_PATH") : getenv("P4C_16_INCLUDE_PATH");
        cmd += cstring(" -C -undef -nostdinc -x assembler-with-cpp") + " " + preprocessor_options
            + (driverP4IncludePath ? " -I" + cstring(driverP4IncludePath) : "")
            + " -I" + (isv1() ? p4_14includePath : p4includePath) + " " + file;

        if (Log::verbose())
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
        if (WIFEXITED(exitCode) && WEXITSTATUS(exitCode) == 4)
            ::error("input file %s does not exist", file);
        else if (exitCode != 0)
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

bool CompilerOptions::enable_intrinsic_metadata_fix() {
    return true;
}

void CompilerOptions::dumpPass(const char* manager, unsigned seq, const char* pass,
                               const IR::Node* node) const {
    if (strncmp(pass, "P4::", 4) == 0) pass += 4;
    cstring name = cstring(manager) + "_" + Util::toString(seq) + "_" + pass;
    if (Log::verbose())
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
                if (Log::verbose())
                    std::cerr << "Writing program to " << fileName << std::endl;
                P4::ToP4 toP4(stream, Log::verbose(), file);
                node->apply(toP4);
                delete stream;  // close the file
            }
            break;
        }
    }
}

DebugHook CompilerOptions::getDebugHook() const {
    using namespace std::placeholders;
    auto dp = std::bind(&CompilerOptions::dumpPass, this, _1, _2, _3, _4);
    return dp;
}

/* static */ P4CContext& P4CContext::get() {
    return CompileContextStack::top<P4CContext>();
}

bool P4CContext::isRecognizedDiagnostic(cstring diagnostic) {
    static const std::unordered_set<cstring> recognizedDiagnostics = {
        "uninitialized_out_param",
        "uninitialized_use",
        "unknown_diagnostic"
    };

    return recognizedDiagnostics.count(diagnostic);
}
