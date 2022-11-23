#ifndef TESTGEN_TARGETS_EBPF_CONCOLIC_H_
#define TESTGEN_TARGETS_EBPF_CONCOLIC_H_

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

class EBPFConcolic : public Concolic {
 private:
    /// This is the list of concolic functions that are implemented in this class.
    static const ConcolicMethodImpls::ImplList EBPFConcolicMethodImpls;

 public:
    /// @returns the concolic  functions that are implemented for this particular target.
    static const ConcolicMethodImpls::ImplList* getEBPFConcolicMethodImpls();
};

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_CONCOLIC_H_ */
