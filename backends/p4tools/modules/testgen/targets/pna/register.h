#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/pna/pna.h"
#include "backends/p4tools/modules/testgen/targets/pna/target.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace P4Tools {

namespace P4Testgen {

/// Register the PNA compiler target with the tools framework.
inline void pna_registerCompilerTarget() { Pna::PnaDpdkCompilerTarget::make(); }

/// Register the PNA testgen target with the testgen framework.
inline void pna_registerTestgenTarget() { Pna::PnaDpdkTestgenTarget::make(); }

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_REGISTER_H_ */
