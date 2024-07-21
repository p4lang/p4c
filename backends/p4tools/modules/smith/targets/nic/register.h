#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_REGISTER_H_

#include "backends/p4tools/modules/smith/targets/nic/target.h"

namespace p4c::P4Tools::P4Smith {

inline void nicRegisterSmithTarget() { Nic::DpdkPnaSmithTarget::make(); }

}  // namespace p4c::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_REGISTER_H_ */
