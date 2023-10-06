#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PNA_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PNA_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

namespace P4Tools::P4Testgen {

namespace Pna {

class PnaDpdkCompilerTarget : public CompilerTarget {
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
