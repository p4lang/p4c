#ifndef _P4_FRONTEND_H_
#define _P4_FRONTEND_H_

#include "ir/ir.h"
#include "../common/options.h"

class FrontEnd {
    std::vector<DebugHook> hooks;
 public:
    FrontEnd() = default;
    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    const IR::P4Program* run(const CompilerOptions& options, const IR::P4Program* program);
};

#endif /* _P4_FRONTEND_H_ */
