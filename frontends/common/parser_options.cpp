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

#include "parser_options.h"

#include <getopt.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <memory>
#include <regex>
#include <unordered_set>

#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/json_generator.h"
#include "lib/exceptions.h"
#include "lib/exename.h"
#include "lib/log.h"
#include "lib/nullstream.h"

namespace P4 {

/* CONFIG_PKGDATADIR is defined by cmake at compile time to be the same as
 * CMAKE_INSTALL_PREFIX This is only valid when the compiler is built and
 * installed from source locally. If the compiled binary is moved to another
 * machine, the 'p4includePath' would contain the path on the original machine.
 */
const char *p4includePath = CONFIG_PKGDATADIR "/p4include";
const char *p4_14includePath = CONFIG_PKGDATADIR "/p4_14include";

using namespace P4::literals;

bool isSystemFile(cstring file) { return file.startsWith(p4includePath); }

void ParserOptions::closeFile(FILE *file) {
    if (file == nullptr) {
        return;
    }
    int exitCode = pclose(file);
    if (WIFEXITED(exitCode) && WEXITSTATUS(exitCode) == 4) {
        ::P4::error(ErrorType::ERR_IO, "input file does not exist");
        return;
    }
    if (exitCode != 0) {
        ::P4::error(ErrorType::ERR_IO, "Preprocessor returned exit code %d; aborting compilation",
                    exitCode);
        return;
    }
}

ParserOptions::ParserOptions(std::string_view defaultMessage) : Util::Options(defaultMessage) {
    registerOption(
        "--help", nullptr,
        [this](const char *) {
            usage();
            exit(0);
            return false;
        },
        "Print this help message");
    registerOption(
        "--version", nullptr,
        [this](const char *) {
            std::cerr << binaryName << std::endl;
            std::cerr << "Version " << compilerVersion << std::endl;
            exit(0);
            return false;
        },
        "Print compiler version");
    registerOption(
        "-I", "path",
        [this](const char *arg) {
            preprocessor_options += std::string(" -I") + arg;
            return true;
        },
        "Specify include path (passed to preprocessor)");
    registerOption(
        "-D", "arg=value",
        [this](const char *arg) {
            preprocessor_options += std::string(" -D") + arg;
            return true;
        },
        "Define macro (passed to preprocessor)");
    registerOption(
        "-U", "arg",
        [this](const char *arg) {
            preprocessor_options += std::string(" -U") + arg;
            return true;
        },
        "Undefine macro (passed to preprocessor)");
    registerOption(
        "-E", nullptr,
        [this](const char *) {
            doNotCompile = true;
            return true;
        },
        "Preprocess only, do not compile (prints program on stdout)");
    registerOption(
        "-M", nullptr,
        [this](const char *) {
            preprocessor_options += std::string(" -M");
            doNotCompile = true;
            return true;
        },
        "Output `make` dependency rule only (passed to preprocessor)");
    registerOption(
        "-MD", nullptr,
        [this](const char *) {
            preprocessor_options += std::string(" -MD");
            return true;
        },
        "Output `make` dependency rule to file as side effect (passed to preprocessor)");
    registerOption(
        "-MF", "file",
        [this](const char *arg) {
            preprocessor_options += std::string(" -MF \"") + arg + std::string("\"");
            return true;
        },
        "With -M, specify output file for dependencies (passed to preprocessor)");
    registerOption(
        "-MG", nullptr,
        [this](const char *) {
            preprocessor_options += std::string(" -MG");
            return true;
        },
        "with -M, suppress errors for missing headers (passed to preprocessor)");
    registerOption(
        "-MP", nullptr,
        [this](const char *) {
            preprocessor_options += std::string(" -MP");
            return true;
        },
        "with -M, add phony target for each dependency (passed to preprocessor)");
    registerOption(
        "-MT", "target",
        [this](const char *arg) {
            preprocessor_options += std::string(" -MT \"") + arg + std::string("\"");
            return true;
        },
        "With -M, override target of the output rule (passed to preprocessor)");
    registerOption(
        "-MQ", "target",
        [this](const char *arg) {
            preprocessor_options += std::string(" -MQ \"") + arg + std::string("\"");
            return true;
        },
        "Like -Mt, override target but quote special characters (passed to preprocessor)");

    registerOption(
        "--p4v", "{14|16}",
        [this](const char *arg) {
            if (!strcmp(arg, "1.0") || !strcmp(arg, "14")) {
                langVersion = ParserOptions::FrontendVersion::P4_14;
            } else if (!strcmp(arg, "1.2") || !strcmp(arg, "16")) {
                langVersion = ParserOptions::FrontendVersion::P4_16;
            } else {
                ::P4::error(ErrorType::ERR_INVALID, "Illegal language version %1%", arg);
                return false;
            }
            return true;
        },
        "[Deprecated; use --std instead] Specify language version to compile.", OptionFlags::Hide);
    registerOption(
        "--std", "{p4-14|p4-16}",
        [this](const char *arg) {
            if (!strcmp(arg, "14") || !strcmp(arg, "p4-14")) {
                langVersion = ParserOptions::FrontendVersion::P4_14;
            } else if (!strcmp(arg, "16") || !strcmp(arg, "p4-16")) {
                langVersion = ParserOptions::FrontendVersion::P4_16;
            } else {
                ::P4::error(ErrorType::ERR_INVALID, "Illegal language version %1%", arg);
                return false;
            }
            return true;
        },
        "Specify language version to compile.");
    registerOption(
        "--nocpp", nullptr,
        [this](const char *) {
            doNotPreprocess = true;
            return true;
        },
        "Skip preprocess, assume input file is already preprocessed.");
    registerOption(
        "--disable-annotations", "annotations",
        [this](const char *arg) {
            auto copy = strdup(arg);
            while (auto name = strsep(&copy, ",")) disabledAnnotations.insert(cstring(name));
            return true;
        },
        "Specify a (comma separated) list of annotations that should be "
        "ignored by\n"
        "the compiler. A warning will be printed that the annotation is "
        "ignored",
        OptionFlags::OptionalArgument);
    registerOption(
        "--Wdisable", "diagnostic",
        [](const char *diagnostic) {
            if (diagnostic) {
                if (ErrorCatalog::getCatalog().isError(diagnostic)) {
                    ::P4::error(ErrorType::ERR_INVALID, "Error %1% cannot be demoted.", diagnostic);
                    return false;
                }
                P4CContext::get().setDiagnosticAction(diagnostic, DiagnosticAction::Ignore);
            } else {
                auto action = DiagnosticAction::Ignore;
                P4CContext::get().setDefaultWarningDiagnosticAction(action);
            }
            return true;
        },
        "Disable a compiler diagnostic, or disable all warnings if no "
        "diagnostic is specified.",
        OptionFlags::OptionalArgument);
    registerOption(
        "--Winfo", "diagnostic",
        [](const char *diagnostic) {
            if (diagnostic) {
                if (ErrorCatalog::getCatalog().isError(diagnostic)) {
                    ::P4::error(ErrorType::ERR_INVALID, "Error %1% cannot be demoted.", diagnostic);
                    return false;
                }
                P4CContext::get().setDiagnosticAction(diagnostic, DiagnosticAction::Info);
            }
            return true;
        },
        "Report an info message for a compiler diagnostic.", OptionFlags::OptionalArgument);
    registerOption(
        "--Wwarn", "diagnostic",
        [](const char *diagnostic) {
            if (diagnostic) {
                if (ErrorCatalog::getCatalog().isError(diagnostic)) {
                    ::P4::error(ErrorType::ERR_INVALID, "Error %1% cannot be demoted.", diagnostic);
                    return false;
                }
                P4CContext::get().setDiagnosticAction(diagnostic, DiagnosticAction::Warn);
            } else {
                auto action = DiagnosticAction::Warn;
                P4CContext::get().setDefaultInfoDiagnosticAction(action);
            }
            return true;
        },
        "Report a warning for a compiler diagnostic, or treat all info messages as "
        "warnings if no diagnostic is specified.",
        OptionFlags::OptionalArgument);
    registerOption(
        "--Werror", "diagnostic",
        [](const char *diagnostic) {
            if (diagnostic) {
                P4CContext::get().setDiagnosticAction(diagnostic, DiagnosticAction::Error);
            } else {
                auto action = DiagnosticAction::Error;
                P4CContext::get().setDefaultWarningDiagnosticAction(action);
            }
            return true;
        },
        "Report an error for a compiler diagnostic, or treat all warnings as "
        "errors if no diagnostic is specified.",
        OptionFlags::OptionalArgument);
    registerOption(
        "--maxErrorCount", "errorCount",
        [](const char *arg) {
            auto maxError = strtoul(arg, nullptr, 10);
            P4CContext::get().errorReporter().setMaxErrorCount(maxError);
            return true;
        },
        "Set the maximum number of errors to display before failing.");
    registerOption(
        "-T", "loglevel",
        [](const char *arg) {
            Log::addDebugSpec(arg);
            return true;
        },
        "[Compiler debugging] Adjust logging level per file (see below)");
    registerOption(
        "-v", nullptr,
        [](const char *) {
            Log::increaseVerbosity();
            return true;
        },
        "[Compiler debugging] Increase verbosity level (can be repeated)");
    registerOption(
        "--top4", "pass1[,pass2]",
        [this](const char *arg) {
            auto copy = strdup(arg);
            while (auto pass = strsep(&copy, ",")) top4.push_back(cstring(pass));
            return true;
        },
        "[Compiler debugging] Dump the P4 representation after\n"
        "passes whose name contains one of `passX' substrings.\n"
        "When '-v' is used this will include the compiler IR.\n");
    registerOption(
        "--dump", "folder",
        [this](const char *arg) {
            dumpFolder = arg;
            return true;
        },
        "[Compiler debugging] Folder where P4 programs are dumped\n");
    registerOption(
        "--parser-inline-opt", nullptr,
        [this](const char *) {
            optimizeParserInlining = true;
            return true;
        },
        "Enable optimization of inlining of callee parsers (subparsers).\n"
        "The optimization is disabled by default.\n"
        "When the optimization is disabled, for each invocation of the subparser\n"
        "all states of the subparser are inlined, which means that the subparser\n"
        "might be inlined multiple times even if it is the same instance\n"
        "which is invoked multiple times.\n"
        "When the optimization is enabled, compiler tries to identify the cases,\n"
        "when it can inline the subparser's states only once for multiple\n"
        "invocations of the same subparser instance.");
    registerOption(
        "--doNotEmitIncludes", "condition",
        [this](const char *arg) {
            noIncludes = cstring(arg);
            return true;
        },
        "[Compiler debugging] If true do not generate #include statements\n");
    registerUsage(
        "loglevel format is: \"sourceFile:level,...,sourceFile:level\"\n"
        "where 'sourceFile' is a compiler source file and "
        "'level' is the verbosity level for LOG messages in that file");
}

void ParserOptions::setInputFile() {
    if (remainingOptions.size() > 1) {
        ::P4::error(ErrorType::ERR_OVERLIMIT, "Only one input file must be specified: %s",
                    cstring::join(remainingOptions.begin(), remainingOptions.end(), ","));
        usage();
        exit(1);
    } else if (remainingOptions.size() == 0) {
        ::P4::error(ErrorType::ERR_EXPECTED, "No input files specified");
        usage();
        exit(1);
    } else {
        file = remainingOptions.at(0);
    }
}

namespace {

bool setIncludePathIfExists(const char *&includePathOut, const char *possiblePath) {
    struct stat st;
    if (!(stat(possiblePath, &st) >= 0 && S_ISDIR(st.st_mode))) return false;
    if (auto path = realpath(possiblePath, NULL))
        includePathOut = path;
    else
        includePathOut = strdup(possiblePath);
    return true;
}

}  // namespace

bool ParserOptions::searchForIncludePath(const char *&includePathOut,
                                         std::vector<cstring> userSpecifiedPaths,
                                         const char *exename) {
    bool found = false;
    char buffer[PATH_MAX];

    BUG_CHECK(strlen(exename) < PATH_MAX, "executable path %1% is too long.", exename);

    snprintf(buffer, sizeof(buffer), "%s", exename);
    if (char *p = strrchr(buffer, '/')) {
        ++p;
        exe_name = cstring(p);

        for (auto path : userSpecifiedPaths) {
            snprintf(p, buffer + sizeof(buffer) - p, "%s", path.c_str());
            if (setIncludePathIfExists(includePathOut, buffer)) {
                LOG3("Setting p4 include path to " << includePathOut);
                found = true;
                break;
            }
        }
    }
    return found;
}

std::vector<const char *> *ParserOptions::process(int argc, char *const argv[]) {
    searchForIncludePath(p4includePath, {"p4include"_cs, "../p4include"_cs, "../../p4include"_cs},
                         exename(argv[0]));
    searchForIncludePath(p4_14includePath,
                         {"p4_14include"_cs, "../p4_14include"_cs, "../../p4_14include"_cs},
                         exename(argv[0]));

    auto remainingOptions = Util::Options::process(argc, argv);
    if (!validateOptions()) {
        return nullptr;
    }
    return remainingOptions;
}

const char *ParserOptions::getIncludePath() const {
    cstring path = cstring::empty;
    // the p4c driver sets environment variables for include
    // paths.  check the environment and add these to the command
    // line for the preprocessor
    char *driverP4IncludePath =
        isv1() ? getenv("P4C_14_INCLUDE_PATH") : getenv("P4C_16_INCLUDE_PATH");
    if (driverP4IncludePath != nullptr) path += (" -I"_cs + cstring(driverP4IncludePath));
    path += " -I"_cs + (isv1() ? p4_14includePath : p4includePath);
    if (!isv1()) path += " -I"_cs + p4includePath + "/bmv2"_cs;
    return path.c_str();
}

std::optional<ParserOptions::PreprocessorResult> ParserOptions::preprocess() const {
    FILE *in = nullptr;

    if (file == "-") {
        in = stdin;
    } else {
#ifdef __clang__
        std::string cmd("cc -E -x c -Wno-comment");
#else
        std::string cmd("cpp");
#endif

        cmd += " -C -undef -nostdinc -x assembler-with-cpp " + preprocessor_options.string() +
               getIncludePath() + " " + absl::CEscape(file.native());

        if (Log::verbose()) std::cerr << "Invoking preprocessor " << std::endl << cmd << std::endl;
        in = popen(cmd.c_str(), "r");
        if (in == nullptr) {
            ::P4::error(ErrorType::ERR_IO, "Error invoking preprocessor");
            perror("");
            return std::nullopt;
        }
    }

    if (doNotCompile) {
        char *line = nullptr;
        size_t len = 0;
        ssize_t read = 0;

        while ((read = getline(&line, &len, in)) != -1) {
            printf("%s", line);
        }
        return std::nullopt;
    }
    return ParserOptions::PreprocessorResult(in, &closeFile);
}

// From (folder, file.ext, suffix)  returns
// folder/file-suffix.ext
static std::filesystem::path makeFileName(const std::filesystem::path &folder,
                                          const std::filesystem::path &name,
                                          std::string_view baseSuffix) {
    std::filesystem::path newName(name.stem());
    newName += baseSuffix;
    newName += name.extension();

    return folder / newName;
}

bool ParserOptions::isv1() const { return langVersion == ParserOptions::FrontendVersion::P4_14; }

void ParserOptions::dumpPass(const char *manager, unsigned seq, const char *pass,
                             const IR::Node *node) const {
    if (strncmp(pass, "P4::", 4) == 0) pass += 4;
    std::string name = absl::StrCat(manager, "_", seq, "_", pass);
    if (Log::verbose()) std::cerr << name << std::endl;

    for (auto s : top4) {
        bool match = false;
        try {
            auto s_regex = std::regex(s, std::regex_constants::ECMAScript);
            // we use regex_search instead of regex_match
            // regex_match compares the regex against the entire string
            // regex_search checks if the regex is contained as substring
            match = std::regex_search(name.begin(), name.end(), s_regex);
        } catch (const std::regex_error &e) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "Malformed toP4 regex string \"%s\".\n"
                        "The regex matcher follows ECMAScript syntax.",
                        s);
            exit(1);
        }
        if (match) {
            std::string suffix = absl::StrFormat("-%04zu-%s", ++dump_uid, name);
            std::filesystem::path fileName =
                makeFileName(dumpFolder, (file == "-" ? "tmp.p4" : file), suffix);

            std::unique_ptr<std::ostream> stream{openFile(fileName, true)};
            if (stream != nullptr) {
                if (Log::verbose()) std::cerr << "Writing program to " << fileName << std::endl;
                // FIXME: Accept path here
                P4::ToP4 toP4(stream.get(), Log::verbose(), cstring(file));
                if (noIncludes) {
                    toP4.setnoIncludesArg(true);
                }
                if (node) {
                    node->apply(toP4);
                } else {
                    *stream << "No P4 program returned by the pass" << std::endl;
                }
            }
            break;
        }
    }
}

bool ParserOptions::isAnnotationDisabled(const IR::Annotation *a) const {
    if (disabledAnnotations.count(a->name.name) > 0) {
        ::P4::warning(ErrorType::WARN_IGNORE, "%1% is ignored because it was explicitly disabled",
                      a);
        return true;
    }
    return false;
}

DebugHook ParserOptions::getDebugHook() const {
    auto dp = std::bind(&ParserOptions::dumpPass, this, std::placeholders::_1,
                        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    return dp;
}

/* static */ P4CContext &P4CContext::get() { return CompileContextStack::top<P4CContext>(); }

const P4CConfiguration &P4CContext::getConfig() {
    if (CompileContextStack::isEmpty()) return DefaultP4CConfiguration::get();
    return get().getConfigImpl();
}

bool P4CContext::isRecognizedDiagnostic(cstring diagnostic) {
    static const std::unordered_set<cstring> recognizedDiagnostics = {
        "uninitialized_out_param"_cs, "uninitialized_use"_cs, "unknown_diagnostic"_cs};

    return recognizedDiagnostics.count(diagnostic);
}
const P4CConfiguration &P4CContext::getConfigImpl() { return DefaultP4CConfiguration::get(); }

}  // namespace P4
