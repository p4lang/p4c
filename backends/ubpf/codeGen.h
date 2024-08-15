#ifndef BACKENDS_UBPF_CODEGEN_H_
#define BACKENDS_UBPF_CODEGEN_H_

#include "backends/ebpf/codeGen.h"
#include "lib/sourceCodeBuilder.h"
#include "target.h"

namespace P4::UBPF {

class UbpfCodeBuilder : public EBPF::CodeBuilder {
 public:
    const UbpfTarget *target;
    explicit UbpfCodeBuilder(const UbpfTarget *target)
        : EBPF::CodeBuilder(target), target(target) {}
};

}  // namespace P4::UBPF

#endif /* BACKENDS_UBPF_CODEGEN_H_ */
