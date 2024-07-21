#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_

#include <string>

#include "backends/p4tools/modules/smith/core/target.h"

namespace p4c::P4Tools::P4Smith::BMv2 {

class AbstractBMv2SmithTarget : public SmithTarget {
 protected:
    explicit AbstractBMv2SmithTarget(const std::string &deviceName, const std::string &archName);
};
}  // namespace p4c::P4Tools::P4Smith::BMv2

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_ */
