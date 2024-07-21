#include "backends/p4tools/modules/testgen/targets/ebpf/concolic.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace p4c::P4Tools::P4Testgen::EBPF {

const ConcolicMethodImpls::ImplList EBPFConcolic::EBPF_CONCOLIC_METHOD_IMPLS{};

const ConcolicMethodImpls::ImplList *EBPFConcolic::getEBPFConcolicMethodImpls() {
    return &EBPF_CONCOLIC_METHOD_IMPLS;
}

}  // namespace p4c::P4Tools::P4Testgen::EBPF
