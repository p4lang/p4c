#ifndef BACKENDS_P4TOOLS_COMMON_OPTIONS_H_
#define BACKENDS_P4TOOLS_COMMON_OPTIONS_H_

#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>

#include "frontends/common/options.h"
#include "lib/compile_context.h"

namespace P4::P4Tools {

/// Encapsulates and processes command-line options for a compiler-based tool. Implementations
/// should use the singleton pattern and define a static get() for obtaining the singleton
/// instance.
class AbstractP4cToolOptions : public CompilerOptions {
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
    /// @returns an EXIT_SUCCESS context on success, EXIT_FAILURE on error.
    int process(const std::vector<const char *> &args);

    /// Command-line arguments to be sent to the compiler. Populated by @process.
    std::vector<const char *> compilerArgs;

    /// Hook for customizing options processing.
    std::vector<const char *> *process(int argc, char *const argv[]) override;

 protected:
    // Self-assignments and copy constructor can only be used by other options.
    AbstractP4cToolOptions &operator=(const AbstractP4cToolOptions &) = default;
    AbstractP4cToolOptions(const AbstractP4cToolOptions &) = default;
    AbstractP4cToolOptions(AbstractP4cToolOptions &&) = default;

    [[nodiscard]] bool validateOptions() const override;

    /// The name of the tool associated with these options.
    [[nodiscard]] const std::string &getToolName() const;

    /// Converts a vector of command-line arguments into the traditional (argc, argv) format.
    static std::tuple<int, char **> convertArgs(const std::vector<const char *> &args);

    explicit AbstractP4cToolOptions(std::string_view toolName, std::string_view message);
};

}  // namespace P4::P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_OPTIONS_H_ */
