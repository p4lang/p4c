#ifndef _BACKENDS_V12TEST_MIDEND_H_
#define _BACKENDS_V12TEST_MIDEND_H_

#include "ir/ir.h"
#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace V12Test {

class MidEnd {
    std::vector<DebugHook> hooks;
 public:
    MidEnd() = default;
    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    IR::ToplevelBlock* process(CompilerOptions& options, const IR::P4Program* program);
};

}   // namespace V12Test

#endif /* _BACKENDS_V12TEST_MIDEND_H_ */
