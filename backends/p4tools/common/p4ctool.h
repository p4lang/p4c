#ifndef BACKENDS_P4TOOLS_COMMON_P4CTOOL_H_
#define BACKENDS_P4TOOLS_COMMON_P4CTOOL_H_

#include <vector>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"
#include "ir/ir.h"

namespace P4Tools {

/// Abstract class for all compiler-based tools. Implementations should instantiate this template
/// on a subclass of AbstractP4cToolOptions.
//
// Because of limitations of templates, method implementations must be inlined here.
template <class Options>
class AbstractP4cTool {
 protected:
    /// Provides the implementation of the tool.
    ///
    /// @param program The P4 program after mid-end processing.
    virtual int mainImpl(const IR::P4Program *program) = 0;

    virtual void registerTarget() = 0;

 public:
    /// @param args
    ///     Contains the path to the executable, followed by the command-line arguments for this
    ///     tool.
    int main(const std::vector<const char *> &args) {
        // Register supported compiler targets.
        registerTarget();

        // Process command-line options.
        auto compileContext = Options::get().process(args);
        if (!compileContext) {
            return 1;
        }

        // Set up the compilation context.
        AutoCompileContext autoContext(*compileContext);

        // Run the compiler to get an IR and invoke the tool.
        const auto program = P4Tools::CompilerTarget::runCompiler();
        if (!program) {
            return 1;
        }
        return mainImpl(*program);
    }
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_P4CTOOL_H_ */
