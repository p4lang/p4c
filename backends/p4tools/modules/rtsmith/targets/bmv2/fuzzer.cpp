#include "backends/p4tools/modules/rtsmith/targets/bmv2/fuzzer.h"

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/rtsmith/core/fuzzer.h"

namespace P4::P4Tools::RtSmith::V1Model {

Bmv2V1ModelFuzzer::Bmv2V1ModelFuzzer(const Bmv2V1ModelProgramInfo &programInfo)
    : P4RuntimeFuzzer(programInfo) {}

const Bmv2V1ModelProgramInfo &Bmv2V1ModelFuzzer::getProgramInfo() const {
    return *P4RuntimeFuzzer::getProgramInfo().checkedTo<Bmv2V1ModelProgramInfo>();
}

InitialConfig Bmv2V1ModelFuzzer::produceInitialConfig() {
    InitialConfig initialConfig;
    initialConfig.push_back(produceWriteRequest(true));
    return initialConfig;
}

UpdateSeries Bmv2V1ModelFuzzer::produceUpdateTimeSeries() {
    UpdateSeries updateSeries;
    size_t maxUpdateCount = getProgramInfo().getFuzzerConfig().getMaxUpdateCount();
    size_t updateCount = Utils::getRandInt(maxUpdateCount);

    for (size_t idx = 0; idx < updateCount; ++idx) {
        auto minUpdateTimeInMicroseconds =
            getProgramInfo().getFuzzerConfig().getMinUpdateTimeInMicroseconds();
        auto maxUpdateTimeInMicroseconds =
            getProgramInfo().getFuzzerConfig().getMaxUpdateTimeInMicroseconds();
        auto microseconds =
            Utils::getRandInt(minUpdateTimeInMicroseconds, maxUpdateTimeInMicroseconds);
        updateSeries.emplace_back(microseconds, produceWriteRequest(false));
    }

    return updateSeries;
}

}  // namespace P4::P4Tools::RtSmith::V1Model
