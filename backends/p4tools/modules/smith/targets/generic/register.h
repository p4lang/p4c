#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_GENERIC_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_GENERIC_REGISTER_H_

#include "backends/p4tools/modules/smith/targets/generic/target.h"

namespace P4C::P4Tools::P4Smith {

inline void genericRegisterSmithTarget() { Generic::GenericCoreSmithTarget::make(); }

}  // namespace P4C::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_GENERIC_REGISTER_H_ */
