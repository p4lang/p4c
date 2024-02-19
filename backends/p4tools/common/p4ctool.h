#ifndef BACKENDS_P4TOOLS_COMMON_P4CTOOL_H_
#define BACKENDS_P4TOOLS_COMMON_P4CTOOL_H_

#include <cstdlib>
#include <type_traits>
#include <vector>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/options.h"

namespace P4Tools {

/// Abstract class for all compiler-based tools. Implementations should instantiate this template
/// on a subclass of AbstractP4cToolOptions.
//
// Because of limitations of templates, method implementations must be inlined here.
template <class Options,
          typename = std::enable_if_t<std::is_base_of_v<AbstractP4cToolOptions, Options>>>
class AbstractP4cTool {
 protected:
    /// Provides the implementation of the tool.
    ///
    /// @param program The P4 program after mid-end processing.
    virtual int mainImpl(const CompilerResult &compilerResult) = 0;

    virtual void registerTarget() = 0;

 public:
    /// @param args
    ///     Contains the path to the executable, followed by the command-line arguments for this
    ///     tool.
    int main(const std::vector<const char *> &args) {
        // Register supported compiler targets.
        registerTarget();

        // Process command-line options.
        auto &toolOptions = Options::get();
        auto compileContext = toolOptions.process(args);
        if (!compileContext) {
            return 1;
        }

        // Set up the compilation context.
        AutoCompileContext autoContext(*compileContext);

        // If not explicitly disabled, print basic information to standard output.
        if (!toolOptions.disableInformationLogging) {
            enableInformationLogging();
        }

        // Print the seed if it has been set.
        if (toolOptions.seed) {
            printInfo("============ Program seed %1% =============\n", *toolOptions.seed);
        }

        // Run the compiler to get an IR and invoke the tool.
        const auto compilerResult = P4Tools::CompilerTarget::runCompiler();
        if (!compilerResult.has_value()) {
            return EXIT_FAILURE;
        }
        return mainImpl(compilerResult.value());
    }
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_P4CTOOL_H_ */
