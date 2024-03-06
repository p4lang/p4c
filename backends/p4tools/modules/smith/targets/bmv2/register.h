#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/modules/smith/smith.h"
#include "backends/p4tools/modules/smith/targets/bmv2/bmv2.h"
#include "backends/p4tools/modules/smith/targets/bmv2/target.h"

namespace P4Tools::P4Smith {

inline void bmv2RegisterCompilerTarget() { BMv2::BMv2V1ModelCompilerTarget::make(); }

inline void bmv2RegisterSmithTarget() { BMv2::BMv2V1modelSmithTarget::make(); }

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_REGISTER_H_ */
