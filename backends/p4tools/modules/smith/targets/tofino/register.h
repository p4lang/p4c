#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_TOFINO_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_TOFINO_REGISTER_H_

#include "backends/p4tools/modules/smith/targets/tofino/target.h"

namespace P4::P4Tools::P4Smith {

inline void tofinoRegisterSmithTarget() { Tofino::TofinoTnaSmithTarget::make(); }

}  // namespace P4::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_TOFINO_REGISTER_H_ */
