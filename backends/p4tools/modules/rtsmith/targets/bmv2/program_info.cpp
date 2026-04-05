#include "backends/p4tools/modules/rtsmith/targets/bmv2/program_info.h"

#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "control-plane/p4RuntimeSerializer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

namespace P4::P4Tools::RtSmith::V1Model {

Bmv2V1ModelProgramInfo::Bmv2V1ModelProgramInfo(const CompilerResult &compilerResult)
    : ProgramInfo(compilerResult, P4::P4RuntimeSerializer::get()->generateP4Runtime(
                                      &compilerResult.getProgram(), cstring("v1model"))) {}

}  // namespace P4::P4Tools::RtSmith::V1Model
