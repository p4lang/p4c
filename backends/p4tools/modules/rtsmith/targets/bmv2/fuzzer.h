#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_FUZZER_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_FUZZER_H_

#include "backends/p4tools/modules/rtsmith/core/fuzzer.h"
#include "backends/p4tools/modules/rtsmith/targets/bmv2/program_info.h"

namespace P4::P4Tools::RtSmith::V1Model {

class Bmv2V1ModelFuzzer : public P4RuntimeFuzzer {
 private:
    /// @returns the program info associated with the current target.
    [[nodiscard]] const Bmv2V1ModelProgramInfo &getProgramInfo() const override;

 public:
    explicit Bmv2V1ModelFuzzer(const Bmv2V1ModelProgramInfo &programInfo);

    InitialConfig produceInitialConfig() override;

    UpdateSeries produceUpdateTimeSeries() override;
};

}  // namespace P4::P4Tools::RtSmith::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_FUZZER_H_ */
