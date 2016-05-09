#ifndef _BACKENDS_BMV2_MIDEND_H_
#define _BACKENDS_BMV2_MIDEND_H_

#include "ir/ir.h"
#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace BMV2 {

class MidEnd {
    std::vector<DebugHook> hooks;

    const IR::P4Program* processV1(CompilerOptions& options, const IR::P4Program* program);
    const IR::P4Program* processV1_2(CompilerOptions& options, const IR::P4Program* program);
 public:
    P4::BlockMap* process(CompilerOptions& options, const IR::P4Program* program);
    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_MIDEND_H_ */
