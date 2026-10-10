// SPDX-FileCopyrightText: 2022 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/common/options.h"

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
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

std::vector<char *> AbstractP4cToolOptions::convertArgs(const std::vector<const char *> &args) {
    std::vector<char *> argv;
    argv.reserve(args.size());
    for (const char *arg : args) {
        argumentStorage.push_back(std::make_shared<std::string>(arg));
        argv.push_back(argumentStorage.back()->data());
    }
    return argv;
}

int AbstractP4cToolOptions::process(const std::vector<const char *> &args) {
    // Compiler expects path to executable as first element in argument list.
    compilerArgs.push_back(args.at(0));

    // Convert to the standard (argc, argv) pair.
    auto argv = convertArgs(args);

    // Delegate to the hook.
    auto *remainingArgs = process(static_cast<int>(argv.size()), argv.data());
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
            // Validate the seed before converting it to uint32_t.
            char *end = nullptr;
            errno = 0;
            unsigned long parsed = std::strtoul(arg, &end, 10);

            if (!std::isdigit(static_cast<unsigned char>(arg[0])) || *end != '\0' ||
                errno == ERANGE || parsed > std::numeric_limits<uint32_t>::max()) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "--seed expects an unsigned 32-bit integer, got '%1%'", arg);
                return false;
            }

            seed = static_cast<uint32_t>(parsed);

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
