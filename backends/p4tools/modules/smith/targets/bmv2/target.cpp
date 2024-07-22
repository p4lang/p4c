#include "backends/p4tools/modules/smith/targets/bmv2/target.h"

#include <cstdlib>
#include <string>

#include "backends/p4tools/modules/smith/core/target.h"

namespace P4C::P4Tools::P4Smith::BMv2 {

/* =============================================================================================
 *  AbstractBMv2SmithTarget implementation
 * ============================================================================================= */

AbstractBMv2SmithTarget::AbstractBMv2SmithTarget(const std::string &deviceName,
                                                 const std::string &archName)
    : SmithTarget(deviceName, archName) {}

}  // namespace P4C::P4Tools::P4Smith::BMv2
