#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/bmv2.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/target.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace P4Tools::P4Testgen {

/// Register the BMv2 compiler target with the tools framework.
inline void bmv2RegisterCompilerTarget() { Bmv2::Bmv2V1ModelCompilerTarget::make(); }

/// Register the BMv2 testgen target with the testgen framework.
inline void bmv2RegisterTestgenTarget() { Bmv2::Bmv2V1ModelTestgenTarget::make(); }

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_REGISTER_H_ */
