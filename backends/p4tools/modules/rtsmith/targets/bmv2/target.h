#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_TARGET_H_

#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "backends/p4tools/modules/rtsmith/core/target.h"
#include "backends/p4tools/modules/rtsmith/targets/bmv2/fuzzer.h"
#include "ir/ir.h"

namespace P4::P4Tools::RtSmith::V1Model {

class Bmv2V1ModelRtSmithTarget : public RtSmithTarget {
 private:
    Bmv2V1ModelRtSmithTarget();

 public:
    /// Registers this target.
    static void make();

 protected:
    const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult, const RtSmithOptions &rtSmithOptions,
        const IR::Declaration_Instance *mainDecl) const override;

    [[nodiscard]] Bmv2V1ModelFuzzer &getFuzzerImpl(const ProgramInfo &programInfo) const override;

    [[nodiscard]] MidEnd mkMidEnd(const CompilerOptions &options) const override;
};

}  // namespace P4::P4Tools::RtSmith::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_TARGET_H_ */
