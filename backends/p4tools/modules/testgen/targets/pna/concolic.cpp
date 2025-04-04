#include "backends/p4tools/modules/testgen/targets/pna/concolic.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace P4::P4Tools::P4Testgen::Pna {

const ConcolicMethodImpls::ImplList PnaDpdkConcolic::PNA_DPDK_CONCOLIC_METHOD_IMPLS{};

const ConcolicMethodImpls::ImplList *PnaDpdkConcolic::getPnaDpdkConcolicMethodImpls() {
    return &PNA_DPDK_CONCOLIC_METHOD_IMPLS;
}

const ConcolicMethodImpls::ImplList PnaP4TCConcolic::PNA_P4TC_CONCOLIC_METHOD_IMPLS{};

const ConcolicMethodImpls::ImplList *PnaP4TCConcolic::getPnaP4TCConcolicMethodImpls() {
    return &PNA_P4TC_CONCOLIC_METHOD_IMPLS;
}

}  // namespace P4::P4Tools::P4Testgen::Pna
