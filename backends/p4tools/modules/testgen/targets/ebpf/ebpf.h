#ifndef TESTGEN_TARGETS_EBPF_EBPF_H_
#define TESTGEN_TARGETS_EBPF_EBPF_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

class EBPFCompilerTarget : public CompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    MidEnd mkMidEnd(const CompilerOptions& options) const override;

    EBPFCompilerTarget();
};

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_EBPF_H_ */
