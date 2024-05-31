#ifndef BACKENDS_P4TOOLS_COMMON_OPTIONS_H_
#define BACKENDS_P4TOOLS_COMMON_OPTIONS_H_

// Boost
#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>

#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/options.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for a compiler-based tool. Implementations
/// should use the singleton pattern and define a static get() for obtaining the singleton
/// instance.
class AbstractP4cToolOptions : protected Util::Options {
 private:
    /// The name of the tool associated with these options.
    std::string _toolName;

 public:
    virtual ~AbstractP4cToolOptions() = default;
    /// A seed for the PRNG.
    std::optional<uint32_t> seed = std::nullopt;

    /// Disable information logging.
    bool disableInformationLogging = false;

    /// Processes options.
    ///
    /// @returns a compilation context on success, std::nullopt on error.
    std::optional<ICompileContext *> process(const std::vector<const char *> &args);

    // No copy constructor and no self-assignments.
    AbstractP4cToolOptions(const AbstractP4cToolOptions &) = delete;

    AbstractP4cToolOptions &operator=(const AbstractP4cToolOptions &) = delete;

 protected:
    /// Command-line arguments to be sent to the compiler. Populated by @process.
    std::vector<const char *> compilerArgs;

    /// Hook for customizing options processing.
    std::vector<const char *> *process(int argc, char *const argv[]) override;

    /// Checks if parsed options make sense with respect to each-other.
    /// @returns true if the validation was successfull and false otherwise.
    [[nodiscard]] virtual bool validateOptions() const;

    /// The name of the tool associated with these options.
    [[nodiscard]] const std::string &getToolName() const;

    /// Converts a vector of command-line arguments into the traditional (argc, argv) format.
    static std::tuple<int, char **> convertArgs(const std::vector<const char *> &args);

    explicit AbstractP4cToolOptions(std::string_view toolName, std::string_view message);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_OPTIONS_H_ */
