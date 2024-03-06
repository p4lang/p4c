#include "backends/p4tools/modules/smith/core/target.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "backends/p4tools/modules/smith/targets/bmv2/target.h"
#include "ir/ir.h"

namespace P4Tools::P4Smith::BMv2 {

/* =============================================================================================
 *  AbstractBMv2SmithTarget implementation
 * ============================================================================================= */

AbstractBMv2SmithTarget::AbstractBMv2SmithTarget(std::string deviceName, std::string archName)
    : SmithTarget(deviceName, archName) {}

/* =============================================================================================
 *  Bmv2V1modelSmithTarget implementation
 * ============================================================================================= */

BMv2V1modelSmithTarget::BMv2V1modelSmithTarget() : AbstractBMv2SmithTarget("bmv2", "v1model") {}

void BMv2V1modelSmithTarget::make() {
    static BMv2V1modelSmithTarget *instance = nullptr;
    if (instance == nullptr) {
        instance = new BMv2V1modelSmithTarget();
    }
}

}  // namespace P4Tools::P4Smith::BMv2
