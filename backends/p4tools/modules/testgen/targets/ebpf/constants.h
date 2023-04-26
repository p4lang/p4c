#ifndef TESTGEN_TARGETS_EBPF_CONSTANTS_H_
#define TESTGEN_TARGETS_EBPF_CONSTANTS_H_

#include "ir/ir.h"

namespace P4Tools::P4Testgen::EBPF {

class EBPFConstants {
 public:
    /// eBPF-internal drop variable.
    static const IR::PathExpression ACCEPT_VAR;
};

}  // namespace P4Tools::P4Testgen::EBPF

#endif /* TESTGEN_TARGETS_EBPF_CONSTANTS_H_ */
