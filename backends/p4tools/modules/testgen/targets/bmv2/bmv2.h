#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

namespace P4Tools::P4Testgen::Bmv2 {

class Bmv2V1ModelCompilerTarget : public CompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    MidEnd mkMidEnd(const CompilerOptions &options) const override;

    Bmv2V1ModelCompilerTarget();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_ */
