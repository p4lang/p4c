#include "backends/p4tools/modules/testgen/targets/ebpf/concolic.h"

#include "p4tools/testgen/lib/concolic.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

const ConcolicMethodImpls::ImplList EBPFConcolic::EBPFConcolicMethodImpls{};

const ConcolicMethodImpls::ImplList* EBPFConcolic::getEBPFConcolicMethodImpls() {
    return &EBPFConcolicMethodImpls;
}

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools
