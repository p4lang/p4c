#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_REGISTER_H_

#include "backends/p4tools/modules/smith/targets/bmv2/psa.h"
#include "backends/p4tools/modules/smith/targets/bmv2/v1model.h"

namespace P4Tools::P4Smith {

inline void bmv2RegisterSmithTarget() {
    BMv2::Bmv2V1modelSmithTarget::make();
    BMv2::Bmv2PsaSmithTarget::make();
}

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_REGISTER_H_ */
