#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_BMV2_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_BMV2_H_

#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"
#include "lib/compile_context.h"

namespace P4Tools::P4Smith::BMv2 {

class AbstractBMv2CompilerTarget : public CompilerTarget {
 protected:
    explicit AbstractBMv2CompilerTarget(std::string deviceName, std::string archName);
};

class BMv2V1ModelCompilerTarget : public AbstractBMv2CompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    BMv2V1ModelCompilerTarget();
};

}  // namespace P4Tools::P4Smith::BMv2

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_BMV2_H_ */
