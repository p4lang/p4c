#include "backends/p4tools/modules/rtsmith/targets/bmv2/target.h"

#include "backends/p4tools/modules/rtsmith/targets/bmv2/fuzzer.h"
#include "backends/p4tools/modules/rtsmith/targets/bmv2/program_info.h"
#include "ir/ir.h"

namespace P4::P4Tools::RtSmith::V1Model {

/* =============================================================================================
 *  Bmv2V1ModelRtSmithTarget implementation
 * ============================================================================================= */

Bmv2V1ModelRtSmithTarget::Bmv2V1ModelRtSmithTarget() : RtSmithTarget("bmv2", "v1model") {}

void Bmv2V1ModelRtSmithTarget::make() {
    static Bmv2V1ModelRtSmithTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Bmv2V1ModelRtSmithTarget();
    }
}

MidEnd Bmv2V1ModelRtSmithTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    // Currently a no-op because we have all the necessary information from the front-end.
    midEnd.addPasses({});
    return midEnd;
}

const ProgramInfo *Bmv2V1ModelRtSmithTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const RtSmithOptions &rtSmithOptions,
    const IR::Declaration_Instance * /*mainDecl*/) const {
    auto bmv2V1ModelProgramInfo = new Bmv2V1ModelProgramInfo(compilerResult);
    // Override the fuzzer configurations if a TOML file is provided.
    if (rtSmithOptions.fuzzerConfigPath().has_value())
        bmv2V1ModelProgramInfo->loadFuzzerConfig(rtSmithOptions.fuzzerConfigPath().value());
    // Override the fuzzer configurations if a string representation of the configurations of format
    // TOML is provided.
    else if (rtSmithOptions.fuzzerConfigString().has_value()) {
        bmv2V1ModelProgramInfo->loadFuzzerConfigInString(
            rtSmithOptions.fuzzerConfigString().value().c_str());
    }
    return bmv2V1ModelProgramInfo;
}

Bmv2V1ModelFuzzer &Bmv2V1ModelRtSmithTarget::getFuzzerImpl(const ProgramInfo &programInfo) const {
    return *new Bmv2V1ModelFuzzer(*programInfo.checkedTo<Bmv2V1ModelProgramInfo>());
}

}  // namespace P4::P4Tools::RtSmith::V1Model
