#include "backends/p4tools/modules/testgen/targets/ebpf/concolic.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace P4Tools::P4Testgen::EBPF {

const ConcolicMethodImpls::ImplList EBPFConcolic::EBPF_CONCOLIC_METHOD_IMPLS{};

const ConcolicMethodImpls::ImplList *EBPFConcolic::getEBPFConcolicMethodImpls() {
    return &EBPF_CONCOLIC_METHOD_IMPLS;
}

}  // namespace P4Tools::P4Testgen::EBPF
