#ifndef TESTGEN_TARGETS_BMV2_REGISTER_H_
#define TESTGEN_TARGETS_BMV2_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/testgen/options.h"
#include "backends/p4tools/testgen/targets/bmv2/bmv2.h"
#include "backends/p4tools/testgen/targets/bmv2/target.h"
#include "backends/p4tools/testgen/testgen.h"

namespace P4Tools {

namespace P4Testgen {

void bmv2_registerCompilerTarget() { Bmv2::BMv2_V1ModelCompilerTarget::make(); }

void bmv2_registerTestgenTarget() { Bmv2::BMv2_V1ModelTestgenTarget::make(); }

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_REGISTER_H_ */
