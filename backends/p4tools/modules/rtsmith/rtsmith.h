#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_RTSMITH_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_RTSMITH_H_

#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/modules/rtsmith/core/fuzzer.h"
#include "backends/p4tools/modules/rtsmith/options.h"

namespace P4::P4Tools::RtSmith {

struct RtSmithResult {
    InitialConfig config;
    UpdateSeries updateSeries;

    RtSmithResult(InitialConfig &&config, UpdateSeries &&updateSeries)
        : config(std::move(config)), updateSeries(std::move(updateSeries)) {}
};

/// This is main implementation of the P4RuntimeSmith tool.
class RtSmith : public AbstractP4cTool<RtSmithOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const CompilerResult &compilerResult) override;

 public:
    virtual ~RtSmith() = default;

    static std::optional<RtSmithResult> generateConfig(const std::string &program,
                                                       const RtSmithOptions &rtSmithOptions);

    static std::optional<RtSmithResult> generateConfig(const RtSmithOptions &rtSmithOptions);

    /// Generate the compiler result for the given program (in order to get a `ProgramInfo` object
    /// later).
    static std::optional<const P4::P4Tools::CompilerResult> generateCompilerResult(
        std::optional<std::reference_wrapper<const std::string>> program,
        const RtSmithOptions &rtSmithOptions);
};

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_RTSMITH_H_ */
