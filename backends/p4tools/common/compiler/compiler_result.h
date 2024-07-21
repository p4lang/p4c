#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_COMPILER_RESULT_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_COMPILER_RESULT_H_

#include <functional>
#include <optional>

#include "ir/ir.h"
#include "lib/castable.h"

namespace p4c::P4Tools {

/// An extensible result object which is returned by the CompilerTarget.
/// In its simplest form, this holds the transformed P4 program after the front- and midend passes.
class CompilerResult : public ICastable {
 private:
    /// The reference to the input P4 program, after it has been transformed by the compiler.
    std::reference_wrapper<const IR::P4Program> program;

 public:
    explicit CompilerResult(const IR::P4Program &program);

    /// @returns the reference to the input P4 program, after it has been transformed by the
    /// compiler.
    [[nodiscard]] const IR::P4Program &getProgram() const;

    DECLARE_TYPEINFO(CompilerResult);
};

/// P4Tools compilers may return an error instead of a compiler result.
/// This is a convenience definition for the return value.
using CompilerResultOrError = std::optional<std::reference_wrapper<const CompilerResult>>;

}  // namespace p4c::P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_COMPILER_RESULT_H_ */
