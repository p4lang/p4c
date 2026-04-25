#include "backends/p4tools/modules/rtsmith/targets/tofino/program_info.h"

#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "control-plane/p4RuntimeSerializer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

namespace P4::P4Tools::RtSmith::Tna {

TofinoTnaProgramInfo::TofinoTnaProgramInfo(const CompilerResult &compilerResult,
                                           const P4::P4RuntimeAPI &p4runtimeApi)
    : ProgramInfo(compilerResult, p4runtimeApi) {}

}  // namespace P4::P4Tools::RtSmith::Tna
