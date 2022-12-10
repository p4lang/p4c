#include "backends/p4tools/modules/testgen/targets/bmv2/bmv2.h"

#include <string>

#include "backends/bmv2/common/annotations.h"
#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "control-plane/addMissingIds.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "frontends/common/options.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

BMv2_V1ModelCompilerTarget::BMv2_V1ModelCompilerTarget() : CompilerTarget("bmv2", "v1model") {}

void BMv2_V1ModelCompilerTarget::make() {
    static BMv2_V1ModelCompilerTarget* INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new BMv2_V1ModelCompilerTarget();
    }
}

MidEnd BMv2_V1ModelCompilerTarget::mkMidEnd(const CompilerOptions& options) const {
    MidEnd midEnd(options);
    auto* refMap = midEnd.getRefMap();
    auto* typeMap = midEnd.getTypeMap();
    midEnd.addPasses({
        // Parse BMv2-specific annotations.
        new BMV2::ParseAnnotations(),
        // Parse P4Runtime-specific annotations and insert missing IDs.
        // Only do this for the protobuf back end.
        TestgenOptions::get().testBackend == "PROTOBUF"
            ? new P4::AddMissingIdAnnotations(
                  refMap, typeMap, new P4::ControlPlaneAPI::Standard::V1ModelArchHandlerBuilder())
            : nullptr,
    });
    midEnd.addDefaultPasses();

    return midEnd;
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
