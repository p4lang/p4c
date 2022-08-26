#include "backends/p4tools/p4check/commands.h"

#include <string.h>

#include <algorithm>
#include <climits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/none.hpp>

#include "lib/exceptions.h"
#include "lib/map.h"

namespace P4Tools {

P4CheckCommands::P4CheckCommands() {
    registerCommand(HELP, {}, {"help", "-h", "--help"}, "Shows this help message and exits");
    registerCommand(VERSION, {}, {"version", "--version"}, "Prints version information and exits");
    registerCommand(TESTGEN, {"p4testgen"}, {"testgen"}, "Generates packet tests for P4 programs");
    registerCommand(SMITH, {"smith"}, {"smith"}, "Generates random p4 programs");
    registerCommand(MUTATE, {"mutate"}, {"mutate"}, "Mutates the given p4 program");
}
void P4CheckCommands::registerCommand(Command cmd, std::set<std::string>&& exeNames,
                                      std::set<std::string>&& cmdLineNames,
                                      const std::string& desc) {
    BUG_CHECK(!registeredCommands.count(cmd), "Attempted to register a command twice: %d", cmd);

    registeredCommands[cmd] = {desc, cmdLineNames};
    for (const auto& name : exeNames) {
        exeNameCommands[name] = cmd;
    }
    for (const auto& name : cmdLineNames) {
        cmdLineCommands[name] = cmd;
    }
}

P4CheckCommands& P4CheckCommands::get() {
    static P4CheckCommands INSTANCE;
    return INSTANCE;
}

void P4CheckCommands::usage(const char* binaryName) {
    *out << "Statically check a P4 program" << std::endl
         << std::endl
         << "Usage: " << binaryName << " command [options] [arguments]" << std::endl
         << std::endl
         << "Available commands:" << std::endl;

    for (auto& pair : Values(registeredCommands)) {
        auto& desc = pair.first;
        auto& names = pair.second;
        *out << std::endl;
        *out << "  ";

        // Output names for the current command.
        bool needSep = false;
        for (const auto& name : names) {
            *out << (needSep ? ", " : "") << name;
            needSep = true;
        }
        *out << std::endl;

        // Output description for the current command.
        std::string line;
        std::stringstream ss(desc);
        while (std::getline(ss, line)) {
            *out << "    " << line << std::endl;
        }
    }
}

const std::vector<const char*>& P4CheckCommands::tokenizeExeName(const char* argv0) {
    static char buffer[PATH_MAX];
    static std::vector<const char*> result;

    if (!result.empty()) {
        return result;  // Done already.
    }

    strncpy(buffer, argv0, PATH_MAX - 1);

    // Seek past the last '/'.
    auto* p = strrchr(buffer, '/');
    if (p != nullptr) {
        ++p;
    } else {
        p = buffer;
    }

    // Split p apart by '-'. Do this by replacing all instances of '-' with 0 and storing the start
    // of each substring into the result. On each iteration of this loop, we add the current
    // substring to the result and scan until we hit '-' or 0. If we see 0, then we've reached the
    // end of the string and are done. Otherwise, we replace '-' with 0 and proceed to the next
    // iteration.
    while (true) {
        result.push_back(p);
        while (*p != '-' && *p != 0) {
            ++p;
        }
        if (*p == 0) {
            break;
        }
        *(p++) = 0;
    }

    return result;
}

const char* P4CheckCommands::getExeNameToken(const char* argv0, size_t n) {
    const auto& tokens = tokenizeExeName(argv0);
    if (n < tokens.size()) {
        return tokens.at(n);
    }
    return nullptr;
}

boost::optional<std::pair<Command, const std::vector<const char*>>> P4CheckCommands::process(
    int argc, char* const argv[]) {
    std::vector<const char*> args;
    for (int i = 0; i < argc; ++i) args.push_back(argv[i]);

    // Figure out the command being invoked.
    boost::optional<Command> command;
    if (const auto* commandName = getExeNameToken(argv[0], 0)) {
        if (exeNameCommands.count(commandName) != 0U) {
            command = exeNameCommands.at(commandName);
        }
    }

    // If we couldn't figure out based on the name of the executable, fall back on the first
    // command-line argument.
    if (!command && argc >= 2 && (cmdLineCommands.count(argv[1]) != 0U)) {
        command = cmdLineCommands.at(argv[1]);

        // Consume the command.
        args.erase(args.begin() + 1);
    }

    // If we still couldn't figure out the command and no command-line options were given, default
    // to "help".
    if (!command && argc == 1) {
        command = HELP;
    }

    // Command-line error if we still don't have a command at this point.
    if (!command) {
        std::cerr << "Error: Invalid command: \"" << argv[1] << "\".Help:\n-------------\n"
                  << std::endl;
        usage(argv[0]);
        return boost::none;
    }

    // Stuff some extra command-line arguments if we can derive the language, device, and
    // architecture from the name of the executable.
    std::vector<const char*> extraArgs;
    if (const auto* langVer = getExeNameToken(argv[0], 1)) {
        extraArgs.push_back("--std");
        extraArgs.push_back(langVer);
    }
    if (const auto* target = getExeNameToken(argv[0], 2)) {
        extraArgs.push_back("--target");
        extraArgs.push_back(target);
    }
    if (const auto* arch = getExeNameToken(argv[0], 3)) {
        extraArgs.push_back("--arch");
        extraArgs.push_back(arch);
    }
    args.insert(args.begin() + 1, extraArgs.begin(), extraArgs.end());

    std::pair<Command, const std::vector<const char*>> result = {*command, args};
    return boost::make_optional(result);
}

}  // namespace P4Tools
