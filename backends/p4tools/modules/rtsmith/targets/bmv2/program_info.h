#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_PROGRAM_INFO_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "ir/ir.h"
#include "ir/node.h"

namespace P4::P4Tools::RtSmith::V1Model {

class Bmv2V1ModelProgramInfo : public ProgramInfo {
 public:
    explicit Bmv2V1ModelProgramInfo(const CompilerResult &compilerResult);

    DECLARE_TYPEINFO(Bmv2V1ModelProgramInfo);
};

}  // namespace P4::P4Tools::RtSmith::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_BMV2_PROGRAM_INFO_H_ */
