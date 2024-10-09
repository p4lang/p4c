#include "backends/p4tools/common/options.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"
#include "lib/error.h"

namespace P4::P4Tools {

std::tuple<int, char **> AbstractP4cToolOptions::convertArgs(
    const std::vector<const char *> &args) {
    int argc = 0;
    char **argv = new char *[args.size()];
    for (const char *arg : args) {
        argv[argc++] = strdup(arg);
    }
    return {argc, argv};
}

int AbstractP4cToolOptions::process(const std::vector<const char *> &args) {
    // Compiler expects path to executable as first element in argument list.
    compilerArgs.push_back(args.at(0));

    // Convert to the standard (argc, argv) pair.
    int argc = 0;
    char **argv = nullptr;
    std::tie(argc, argv) = convertArgs(args);

    // Delegate to the hook.
    auto *remainingArgs = process(argc, argv);
    if ((remainingArgs == nullptr) || errorCount() > 0) {
        return EXIT_FAILURE;
    }
    setInputFile();
    if (!validateOptions()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

std::vector<const char *> *AbstractP4cToolOptions::process(int argc, char *const argv[]) {
    return ParserOptions::process(argc, argv);
}

AbstractP4cToolOptions::AbstractP4cToolOptions(std::string_view toolName, std::string_view message)
    : CompilerOptions(message), _toolName(toolName) {
    // Register some common options.
    registerOption(
        "--seed", "seed",
        [this](const char *arg) {
            seed = std::stoul(arg);
            // Initialize the global seed for randomness.
            Utils::setRandomSeed(*seed);
            return true;
        },
        "Provides a randomization seed");

    registerOption(
        "--disable-info-logging", nullptr,
        [this](const char * /*arg*/) {
            disableInformationLogging = true;
            return true;
        },
        "Disable printing of information messages to standard output.");
}

bool AbstractP4cToolOptions::validateOptions() const { return true; }

const std::string &AbstractP4cToolOptions::getToolName() const { return _toolName; }

}  // namespace P4::P4Tools
