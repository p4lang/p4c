#include "backends/p4tools/modules/testgen/targets/pna/pna.h"

#include <string>

#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

namespace P4Tools::P4Testgen::Pna {

PnaDpdkCompilerTarget::PnaDpdkCompilerTarget() : TestgenCompilerTarget("dpdk", "pna") {}

void PnaDpdkCompilerTarget::make() {
    static PnaDpdkCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new PnaDpdkCompilerTarget();
    }
}

MidEnd PnaDpdkCompilerTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    midEnd.addPasses({});
    midEnd.addDefaultPasses();

    return midEnd;
}

}  // namespace P4Tools::P4Testgen::Pna
