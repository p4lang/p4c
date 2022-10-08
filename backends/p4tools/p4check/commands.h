#ifndef P4CHECK_COMMANDS_H_
#define P4CHECK_COMMANDS_H_

#include <stddef.h>

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/optional/optional.hpp>

namespace P4Tools {

/// Enumerates the commands available in p4check.
enum Command { HELP = 0, VERSION = 1, TESTGEN = 2, VERIFY = 3, MUTATE = 4, SMITH = 5 };

/// A singleton class that processes and encapsulates available commands for p4check.
class P4CheckCommands {
 public:
    /// Returns the singleton instance.
    static P4CheckCommands& get();

    /// Processes the given command-line options. On success, returns (1) the chosen command and
    /// (2) the executable name followed by the command-line arguments for that command. This
    /// second component will be suitable for processing by AbstractP4cToolOptions::process. On
    /// failure, returns boost::none.
    boost::optional<std::pair<Command, const std::vector<const char*>>> process(int argc,
                                                                                char* const argv[]);

    /// Prints usage information to @out.
    void usage(const char* binaryName);

 private:
    /// Messages will be emitted on this stream.
    std::ostream* out = &std::cerr;

    /// Descriptions of each registered command, as well as their names as command-line arguments.
    std::map<Command, std::pair<std::string, std::set<std::string>>> registeredCommands;

    /// Registered commands, indexed by executable name.
    std::map<std::string, Command> exeNameCommands;

    /// Registered commands, indexed by their names as command-line arguments.
    std::map<std::string, Command> cmdLineCommands;

    /// Registers a new command. Should be called exactly once for each available command.
    ///
    /// @param cmd
    ///     identifies the command being registered
    ///
    /// @param exeNames
    ///     the set of executable names that may be associated with the command.
    ///
    /// @param cmdLineNames
    ///     the set of names associated with the command, as they appear on the command line. For
    ///     example, if "foo" is included here, then "p4check foo" will be associated with the
    ///     command being registered.
    void registerCommand(Command cmd, std::set<std::string>&& exeNames,
                         std::set<std::string>&& cmdLineNames, const std::string& desc);

    /// Takes the filename component of the given path, separates the name by hyphens, and returns
    /// the result.
    static const std::vector<const char*>& tokenizeExeName(const char* argv0);

    /// Tokenizes the executable name and returns the @n-th token. Returns null if there are fewer
    /// than @n components. Token indices are 0-based.
    static const char* getExeNameToken(const char* argv0, size_t n);

    P4CheckCommands();

    // No copy constructor and no self-assignments.
    P4CheckCommands(const P4CheckCommands&) = delete;
    P4CheckCommands& operator=(const P4CheckCommands&) = delete;
};

}  // namespace P4Tools

#endif /* P4CHECK_COMMANDS_H_ */
