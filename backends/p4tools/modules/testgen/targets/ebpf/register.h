#ifndef TESTGEN_TARGETS_EBPF_REGISTER_H_
#define TESTGEN_TARGETS_EBPF_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/ebpf.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/target.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace P4Tools::P4Testgen {

/// Register the ebpf compiler target with the tools framework.
inline void ebpfRegisterCompilerTarget() { EBPF::EBPFCompilerTarget::make(); }

/// Register the ebpf testgen target with the testgen framework.
inline void ebpfRegisterTestgenTarget() { EBPF::EBPFTestgenTarget::make(); }

}  // namespace P4Tools::P4Testgen

#endif /* TESTGEN_TARGETS_EBPF_REGISTER_H_ */
