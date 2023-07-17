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
 public:
    /// A seed for the PRNG.
    std::optional<uint32_t> seed = std::nullopt;

    std::optional<ICompileContext *> processToolOptions(int argc, char **argv);

    virtual ~AbstractP4cToolOptions() = default;

    // No copy constructor and no self-assignments.
    AbstractP4cToolOptions(const AbstractP4cToolOptions &) = delete;
    AbstractP4cToolOptions &operator=(const AbstractP4cToolOptions &) = delete;

 protected:
    /// Command-line arguments to be sent to the compiler. Populated by @process.
    std::vector<const char *> compilerArgs;

    explicit AbstractP4cToolOptions(cstring message);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_OPTIONS_H_ */
