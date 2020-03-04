#ifndef P4C_CODEGEN_H
#define P4C_CODEGEN_H

#include "lib/sourceCodeBuilder.h"
#include "ebpf/codeGen.h"
#include "target.h"

namespace UBPF {
    class UbpfCodeBuilder : public EBPF::CodeBuilder {
    public:
        const UbpfTarget *target;
        explicit UbpfCodeBuilder(const UbpfTarget *target) : EBPF::CodeBuilder(target), target(target) {}
    };
}


#endif //P4C_CODEGEN_H
