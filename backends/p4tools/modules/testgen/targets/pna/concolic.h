#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_CONCOLIC_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_CONCOLIC_H_

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace P4Tools::P4Testgen::Pna {

class PnaDpdkConcolic : public Concolic {
 private:
    /// This is the list of concolic functions that are implemented in this class.
    static const ConcolicMethodImpls::ImplList PNA_DPDK_CONCOLIC_METHOD_IMPLS;

 public:
    /// @returns the concolic  functions that are implemented for this particular target.
    static const ConcolicMethodImpls::ImplList *getPnaDpdkConcolicMethodImpls();
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_CONCOLIC_H_ */
