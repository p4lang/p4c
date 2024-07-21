#include "backends/p4tools/modules/testgen/targets/pna/concolic.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace p4c::P4Tools::P4Testgen::Pna {

const ConcolicMethodImpls::ImplList PnaDpdkConcolic::PNA_DPDK_CONCOLIC_METHOD_IMPLS{};

const ConcolicMethodImpls::ImplList *PnaDpdkConcolic::getPnaDpdkConcolicMethodImpls() {
    return &PNA_DPDK_CONCOLIC_METHOD_IMPLS;
}

}  // namespace p4c::P4Tools::P4Testgen::Pna
