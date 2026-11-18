#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_TOFINO_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_TOFINO_PROGRAM_INFO_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/modules/rtsmith/core/program_info.h"
#include "ir/ir.h"
#include "ir/node.h"

namespace P4::P4Tools::RtSmith::Tna {

class TofinoTnaProgramInfo : public ProgramInfo {
 public:
    explicit TofinoTnaProgramInfo(const CompilerResult &compilerResult,
                                  const P4::P4RuntimeAPI &p4runtimeApi);

    DECLARE_TYPEINFO(TofinoTnaProgramInfo);
};

}  // namespace P4::P4Tools::RtSmith::Tna

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_TARGETS_TOFINO_PROGRAM_INFO_H_ */
