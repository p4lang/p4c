#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_REGISTER_H_

#include "backends/p4tools/modules/rtsmith/targets/bmv2/target.h"

namespace P4::P4Tools::RtSmith {

/// Register the V1Model RtSmith target with the P4RuntimeSmith framework.
inline void bmv2_registerRtSmithTarget() { V1Model::Bmv2V1ModelRtSmithTarget::make(); }

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_REGISTER_H_ */
