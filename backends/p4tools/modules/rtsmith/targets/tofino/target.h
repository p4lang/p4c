#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_TOFINO_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_TOFINO_TARGET_H_

#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "backends/p4tools/modules/rtsmith/core/target.h"
#include "backends/p4tools/modules/rtsmith/targets/tofino/fuzzer.h"
#include "ir/ir.h"

namespace P4::P4Tools::RtSmith::Tna {

class TofinoTnaRtSmithTarget : public RtSmithTarget {
 private:
    TofinoTnaRtSmithTarget();

 public:
    /// Registers this target.
    static void make();

 protected:
    const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult, const RtSmithOptions &rtSmithOptions,
        const IR::Declaration_Instance *mainDecl) const override;

    [[nodiscard]] TofinoTnaFuzzer &getFuzzerImpl(const ProgramInfo &programInfo) const override;

    [[nodiscard]] MidEnd mkMidEnd(const CompilerOptions &options) const override;
};

}  // namespace P4::P4Tools::RtSmith::Tna

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_TOFINO_TARGET_H_ */
