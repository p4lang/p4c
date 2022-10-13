#ifndef BACKENDS_P4TOOLS_COMMON_OPTIONS_H_
#define BACKENDS_P4TOOLS_COMMON_OPTIONS_H_

// Boost
#include <cstdint>
#include <tuple>
#include <vector>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/options.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for a compiler-based tool. Implementations
/// should use the singleton pattern and define a static get() for obtaining the singleton
/// instance.
class AbstractP4cToolOptions : protected Util::Options {
 public:
    /// The network's MTU, in bytes.
    int networkMtu_bytes = 1500;

    /// The minimum packet length allowed by the network, in bytes.
    int minPacketSize_bytes = 64;

    /// A seed for the PRNG.
    boost::optional<uint32_t> seed = boost::none;

    /// Build a DCG for input program. This control flow graph directed cyclic graph can be used
    /// for statement reachability analysis.
    bool dcg = false; 

    /// Processes options.
    ///
    /// @returns a compilation context on success, boost::none on error.
    boost::optional<ICompileContext*> process(const std::vector<const char*>& args);

 protected:
    /// Command-line arguments to be sent to the compiler. Populated by @process.
    std::vector<const char*> compilerArgs;

    /// Hook for customizing options processing.
    std::vector<const char*>* process(int argc, char* const argv[]) override;

    /// Converts a vector of command-line arguments into the traditional (argc, argv) format.
    static std::tuple<int, char**> convertArgs(const std::vector<const char*>& args);

    explicit AbstractP4cToolOptions(cstring message);

    // No copy constructor and no self-assignments.
    AbstractP4cToolOptions(const AbstractP4cToolOptions&) = delete;
    AbstractP4cToolOptions& operator=(const AbstractP4cToolOptions&) = delete;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_OPTIONS_H_ */
