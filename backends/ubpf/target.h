#ifndef P4C_TARGET_H
#define P4C_TARGET_H

#include "backends/ebpf/target.h"

namespace UBPF {

class UbpfTarget : public EBPF::KernelSamplesTarget {
 public:
    UbpfTarget() : KernelSamplesTarget("UBPF") {}
    void emitIncludes(Util::SourceCodeBuilder* builder) const override;
    void emitMain(Util::SourceCodeBuilder* builder,
                  cstring functionName,
                  cstring argName) const override;
    void emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    cstring dropReturnCode() const override { return "1"; }
    cstring abortReturnCode() const override { return "1"; }
    cstring forwardReturnCode() const override { return "0"; }
};

}

#endif //P4C_TARGET_H
