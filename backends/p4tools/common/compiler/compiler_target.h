#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_COMPILER_TARGET_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_COMPILER_TARGET_H_

#include <string>
#include <vector>

#include "backends/p4tools/common/compiler/compiler_result.h"
#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/core/target.h"
#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/frontend.h"
#include "ir/ir.h"
#include "lib/compile_context.h"

namespace P4Tools {

/// Encapsulates the details of invoking the P4 compiler for a target device and architecture.
class CompilerTarget : public Target {
 public:
    /// @returns a new compilation context for the compiler.
    static ICompileContext *makeContext(std::string_view toolName);

    /// Initializes the P4 compiler with the given compiler-specific command-line arguments.
    ///
    /// @returns any unprocessed arguments, or nullptr if there was an error.
    static std::vector<const char *> *initCompiler(std::string_view toolName, int argc,
                                                   char **argv);

    /// Runs the P4 compiler to produce an IR and various other kinds of information on the input
    /// program.
    ///
    /// @returns std::nullopt if an error occurs during compilation.
    static CompilerResultOrError runCompiler(const CompilerOptions &options,
                                             std::string_view toolName);

    /// Runs the P4 compiler to produce an IR and other information for the given source code.
    ///
    /// @returns std::nullopt if an error occurs during compilation.
    static CompilerResultOrError runCompiler(const CompilerOptions &options,
                                             std::string_view toolName, const std::string &source);

 private:
    /// Runs the front and mid ends on the given parsed program.
    ///
    /// @returns std::nullopt if an error occurs during compilation.
    static CompilerResultOrError runCompiler(const CompilerOptions &options,
                                             std::string_view toolName, const IR::P4Program *);

 protected:
    /// @see @makeContext.
    [[nodiscard]] virtual ICompileContext *makeContextImpl() const;

    /// @see runCompiler.
    virtual CompilerResultOrError runCompilerImpl(const CompilerOptions &options,
                                                  const IR::P4Program *) const;

    /// This implementation just forwards the given arguments to the compiler.
    ///
    /// @see @initCompiler.
    virtual std::vector<const char *> *initCompilerImpl(int argc, char **argv) const;

    /// Parses the P4 program specified on the command line.
    ///
    /// @returns nullptr if an error occurs during parsing.
    static const IR::P4Program *runParser(const ParserOptions &options);

    /// Runs the front end of the P4 compiler on the given program.
    ///
    /// @returns nullptr if an error occurs during compilation.
    const IR::P4Program *runFrontend(const CompilerOptions &options,
                                     const IR::P4Program *program) const;

    /// A factory method for providing a target-specific mid end implementation.
    [[nodiscard]] virtual MidEnd mkMidEnd(const CompilerOptions &options) const;

    /// A factory method for providing a target-specific front end implementation.
    [[nodiscard]] virtual P4::FrontEnd mkFrontEnd() const;

    /// Runs the mid end provided by @mkMidEnd on the given program.
    ///
    /// @returns nullptr if an error occurs during compilation.
    const IR::P4Program *runMidEnd(const CompilerOptions &options,
                                   const IR::P4Program *program) const;

    explicit CompilerTarget(std::string_view toolName, const std::string &deviceName,
                            const std::string &archName);

 private:
    /// @returns the singleton instance for the current target.
    static const CompilerTarget &get(std::string_view toolName);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_COMPILER_TARGET_H_ */
