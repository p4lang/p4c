#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PNA_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PNA_H_

#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

#include "backends/p4tools/modules/testgen/core/compiler_target.h"

namespace P4Tools::P4Testgen {

namespace Pna {

class PnaDpdkCompilerTarget : public TestgenCompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    MidEnd mkMidEnd(const CompilerOptions &options) const override;

    PnaDpdkCompilerTarget();
};

}  // namespace Pna

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PNA_H_ */
