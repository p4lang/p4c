#include "backends/p4tools/modules/testgen/targets/ebpf/ebpf.h"

#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

EBPFCompilerTarget::EBPFCompilerTarget() : CompilerTarget("ebpf", "ebpf") {}

void EBPFCompilerTarget::make() {
    static EBPFCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new EBPFCompilerTarget();
    }
}

MidEnd EBPFCompilerTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    midEnd.addPasses({});
    midEnd.addDefaultPasses();

    return midEnd;
}

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools
