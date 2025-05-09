#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_EBPF_CONSTANTS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_EBPF_CONSTANTS_H_

#include "ir/ir.h"

namespace P4::P4Tools::P4Testgen::EBPF {

class EBPFConstants {
 public:
    /// eBPF-internal drop variable.
    static const IR::PathExpression ACCEPT_VAR;
    /// Port width in bits.
    static constexpr int PORT_BIT_WIDTH = 9;
};

}  // namespace P4::P4Tools::P4Testgen::EBPF

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_EBPF_CONSTANTS_H_ */
