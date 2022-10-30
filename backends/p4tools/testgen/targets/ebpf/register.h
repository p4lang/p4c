#ifndef TESTGEN_TARGETS_EBPF_REGISTER_H_
#define TESTGEN_TARGETS_EBPF_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/testgen/options.h"
#include "backends/p4tools/testgen/targets/ebpf/ebpf.h"
#include "backends/p4tools/testgen/targets/ebpf/target.h"
#include "backends/p4tools/testgen/testgen.h"

namespace P4Tools {

namespace P4Testgen {

/// Register the ebpf compiler target with the tools framework.
void ebpf_registerCompilerTarget() { EBPF::EBPFCompilerTarget::make(); }

/// Register the ebpf testgen target with the testgen framework.
void ebpf_registerTestgenTarget() { EBPF::EBPFTestgenTarget::make(); }

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_REGISTER_H_ */
