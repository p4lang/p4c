#include "backends/p4tools/common/options.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/version.h"
#include "frontends/common/parser_options.h"
#include "lib/error.h"
#include "lib/exceptions.h"

namespace P4Tools {

std::tuple<int, char **> AbstractP4cToolOptions::convertArgs(
    const std::vector<const char *> &args) {
    int argc = 0;
    char **argv = new char *[args.size()];
    for (const char *arg : args) {
        argv[argc++] = strdup(arg);
    }
    return {argc, argv};
}

std::optional<ICompileContext *> AbstractP4cToolOptions::process(
    const std::vector<const char *> &args) {
    // Compiler expects path to executable as first element in argument list.
    compilerArgs.push_back(args.at(0));

    // Convert to the standard (argc, argv) pair.
    int argc = 0;
    char **argv = nullptr;
    std::tie(argc, argv) = convertArgs(args);

    // Establish a dummy compilation context so that we can use ::error to report errors while
    // processing command-line options.
    class DummyCompileContext : public BaseCompileContext {
    } dummyContext;
    AutoCompileContext autoDummyContext(&dummyContext);

    // Delegate to the hook.
    auto *remainingArgs = process(argc, argv);
    if ((remainingArgs == nullptr) || ::errorCount() > 0) {
        return std::nullopt;
    }

    // Establish the real compilation context.
    auto *compilerContext = P4Tools::CompilerTarget::makeContext();
    AutoCompileContext autoContext(compilerContext);

    // Initialize the compiler, forwarding any compiler-specific options.
    std::tie(argc, argv) = convertArgs(compilerArgs);
    auto *unprocessedCompilerArgs = P4Tools::CompilerTarget::initCompiler(argc, argv);

    if ((unprocessedCompilerArgs == nullptr) || ::errorCount() > 0) {
        return std::nullopt;
    }
    BUG_CHECK(unprocessedCompilerArgs->empty(), "Compiler did not process all of its arguments: %s",
              cstring::join(unprocessedCompilerArgs->begin(), unprocessedCompilerArgs->end(), " "));

    // Remaining arguments should be source files. Ensure we have exactly one and send it to the
    // compiler.
    if (remainingArgs->size() > 1) {
        ::error("Only one input file can be specified. Duplicate args:\n%1%",
                cstring::join(remainingArgs->begin(), remainingArgs->end(), "\n  "));
        usage();
        return std::nullopt;
    }
    if (remainingArgs->empty()) {
        ::error("No input files specified");
        usage();
        return std::nullopt;
    }
    P4CContext::get().options().file = remainingArgs->at(0);

    return compilerContext;
}

std::vector<const char *> *AbstractP4cToolOptions::process(int argc, char *const argv[]) {
    return Util::Options::process(argc, argv);
}

/// Specifies a command-line option to inherit from the compiler, and any special handling for the
/// option.
struct InheritedCompilerOptionSpec {
    /// The name of the command-line option. For example, "--target".
    const char *option;

    /// A descriptive name for the parameter to the option, or nullptr if the option has no
    /// parameters.
    const char *argName;

    /// A description of the option.
    const char *description;

    /// An optional handler for the option. If provided, this is executed before the option is
    /// forwarded to the compiler. Any argument to the option is provided to the handler. The
    /// handler should return true on successful processing, and false otherwise.
    std::optional<std::function<bool(const char *)>> handler;
};

AbstractP4cToolOptions::AbstractP4cToolOptions(cstring message) : Options(message) {
    // Register some common options.
    registerOption(
        "--help", nullptr,
        [this](const char *) {
            usage();
            exit(0);
            return false;
        },
        "Shows this help message and exits");

    registerOption(
        "--version", nullptr,
        [this](const char *) {
            printVersion(binaryName);
            exit(0);
            return false;
        },
        "Prints version information and exits");

    // Inherit some compiler options, setting them up to be forwarded to the compiler.
    std::vector<InheritedCompilerOptionSpec> inheritedCompilerOptions = {
        {"-I", "path", "Adds the given path to the preprocessor include path", {}},
        {"--Wwarn",
         "diagnostic",
         "Report a warning for a compiler diagnostic, or treat all warnings "
         "as warnings (the default) if no diagnostic is specified.",
         {}},
        {"-D", "arg=value", "Defines a preprocessor symbol", {}},
        {"-U", "arg", "Undefines a preprocessor symbol", {}},
        {"-E", nullptr, "Preprocess only. Prints preprocessed program on stdout.", {}},
        {"--nocpp",
         nullptr,
         "Skips the preprocessor; assumes the input file is already preprocessed.",
         {}},
        {"--std", "{p4-14|p4-16}", "Specifies source language version.", {}},
        {"-T", "loglevel", "Adjusts logging level per file.", {}},
        {"--target", "target", "Specifies the device targeted by the program.",
         std::optional<std::function<bool(const char *)>>{[](const char *arg) {
             if (!P4Tools::Target::setDevice(arg)) {
                 ::error("Unsupported target device: %s", arg);
                 return false;
             }
             return true;
         }}},
        {"--arch", "arch", "Specifies the architecture targeted by the program.",
         std::optional<std::function<bool(const char *)>>{[](const char *arg) {
             if (!P4Tools::Target::setArch(arg)) {
                 ::error("Unsupported architecture: %s", arg);
                 return false;
             }
             return true;
         }}},
        {"--top4",
         "pass1[,pass2]",
         "Dump the P4 representation after\n"
         "passes whose name contains one of `passX' substrings.\n"
         "When '-v' is used this will include the compiler IR.\n",
         {}},
        {"--dump", "folder", "Folder where P4 programs are dumped.", {}},
        {"-v", nullptr, "Increase verbosity level (can be repeated)", {}},
    };

    registerOption(
        "--seed", "seed",
        [this](const char *arg) {
            seed = std::stoul(arg);
            // Initialize the global seed for randomness.
            Utils::setRandomSeed(*seed);
            return true;
        },
        "Provides a randomization seed");

    for (const auto &optionSpec : inheritedCompilerOptions) {
        registerOption(
            optionSpec.option, optionSpec.argName,
            [this, optionSpec](const char *arg) {
                // Add to the list of arguments being forwarded to the compiler.
                compilerArgs.push_back(optionSpec.option);
                if (optionSpec.argName != nullptr) {
                    compilerArgs.push_back(arg);
                }

                // Invoke the handler, if provided.
                if (optionSpec.handler) {
                    return (*optionSpec.handler)(arg);
                }
                return true;
            },
            optionSpec.description);
    }
}

}  // namespace P4Tools
