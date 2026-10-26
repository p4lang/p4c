#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_TARGET_H_

#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/modules/rtsmith/core/fuzzer.h"
#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "backends/p4tools/modules/rtsmith/options.h"
#include "ir/ir.h"

namespace P4::P4Tools::RtSmith {

class RtSmithTarget : public CompilerTarget {
 public:
    /// @returns the singleton instance for the current target.
    static const RtSmithTarget &get();

    /// Produces a @ProgramInfo for the given P4 program.
    ///
    /// @returns nullptr if the program is not supported by this target.
    static const ProgramInfo *produceProgramInfo(const CompilerResult &compilerResult,
                                                 const RtSmithOptions &rtSmithOptions);

    /// @returns the fuzzer that will produce an initial configuration and a series of random write
    /// requests..
    [[nodiscard]] static RuntimeFuzzer &getFuzzer(const ProgramInfo &programInfo);

 protected:
    /// @see @produceProgramInfo.
    [[nodiscard]] virtual const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult, const RtSmithOptions &rtSmithOptions) const;

    /// @see @produceProgramInfo.
    virtual const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult, const RtSmithOptions &rtSmithOptions,
        const IR::Declaration_Instance *mainDecl) const = 0;

    /// @see @getStepper.
    [[nodiscard]] virtual RuntimeFuzzer &getFuzzerImpl(const ProgramInfo &programInfo) const = 0;

    explicit RtSmithTarget(const std::string &deviceName, const std::string &archName);

    [[nodiscard]] ICompileContext *makeContext() const override;

 private:
};

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_TARGET_H_ */
