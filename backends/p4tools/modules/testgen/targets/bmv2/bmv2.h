#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Extends the CompilerResult with the associated P4RuntimeApi
class BMv2V1ModelCompilerResult : public CompilerResult {
 private:
    /// The runtimeAPI inferred from this particular BMv2 V1Model P4 program.
    P4::P4RuntimeAPI p4runtimeApi;

 public:
    explicit BMv2V1ModelCompilerResult(const IR::P4Program &program, P4::P4RuntimeAPI p4runtimeApi);

    [[nodiscard]] const P4::P4RuntimeAPI &getP4RuntimeApi() const;
};

class Bmv2V1ModelCompilerTarget : public CompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    [[nodiscard]] MidEnd mkMidEnd(const CompilerOptions &options) const override;

    CompilerResultOrError runCompilerImpl(const IR::P4Program *program) const override;

    Bmv2V1ModelCompilerTarget();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_ */
