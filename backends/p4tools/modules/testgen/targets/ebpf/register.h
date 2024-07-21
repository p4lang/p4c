#ifndef TESTGEN_TARGETS_EBPF_REGISTER_H_
#define TESTGEN_TARGETS_EBPF_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/target.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace p4c::P4Tools::P4Testgen {

/// Register the ebpf testgen target with the testgen framework.
inline void ebpfRegisterTestgenTarget() { EBPF::EBPFTestgenTarget::make(); }

}  // namespace p4c::P4Tools::P4Testgen

#endif /* TESTGEN_TARGETS_EBPF_REGISTER_H_ */
