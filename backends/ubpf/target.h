#ifndef P4C_TARGET_H
#define P4C_TARGET_H

#include "backends/ebpf/target.h"

namespace UBPF {

class UbpfTarget : public EBPF::KernelSamplesTarget {
 public:
    UbpfTarget() : KernelSamplesTarget("UBPF") {}
    void emitIncludes(Util::SourceCodeBuilder* builder) const override;
};

}

#endif //P4C_TARGET_H
