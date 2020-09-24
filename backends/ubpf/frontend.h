
#ifndef _BACKENDS_UBPF_FRONTEND_H_
#define _BACKENDS_UBPF_FRONTEND_H_

#include "ir/ir.h"
#include "frontends/p4/parseAnnotations.h"

namespace UBPF {

class FrontEnd {
    /// A pass for parsing annotations.
    P4::ParseAnnotations parseAnnotations;

    std::vector<DebugHook> hooks;

public:
    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }

    // If p4c is run with option '--listFrontendPasses', outStream is used for printing passes names
    const IR::P4Program *run(const CompilerOptions &options, const IR::P4Program *program,
                             bool skipSideEffectOrdering = false,
                             std::ostream *outStream = nullptr);
};
}  // namespace UBPF

#endif //_BACKENDS_UBPF_FRONTEND_H_