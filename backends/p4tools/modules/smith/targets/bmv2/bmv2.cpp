#include "backends/p4tools/modules/smith/targets/bmv2/bmv2.h"

#include <string>

#include "backends/p4tools/common/compiler/configuration.h"
#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/core/target.h"
#include "lib/error.h"

namespace P4Tools::P4Smith::BMv2 {

AbstractBMv2CompilerTarget::AbstractBMv2CompilerTarget(std::string deviceName, std::string archName)
    : CompilerTarget(deviceName, archName) {}

/* =============================================================================================
 *  BMv2V1ModelCompilerTarget implementation
 * ============================================================================================= */

BMv2V1ModelCompilerTarget::BMv2V1ModelCompilerTarget()
    : AbstractBMv2CompilerTarget("bmv2", "v1model") {}

void BMv2V1ModelCompilerTarget::make() {
    static BMv2V1ModelCompilerTarget *instance = nullptr;
    if (instance == nullptr) {
        instance = new BMv2V1ModelCompilerTarget();
    }
}

}  // namespace P4Tools::P4Smith::BMv2
