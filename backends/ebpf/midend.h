#ifndef _BACKENDS_EBPF_MIDEND_H_
#define _BACKENDS_EBPF_MIDEND_H_

#include "ir/ir.h"
#include "ebpfOptions.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

namespace EBPF {

class MidEnd {
    std::vector<DebugHook> hooks;
 public:
    P4::ReferenceMap       refMap;
    P4::TypeMap            typeMap;

    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    const IR::ToplevelBlock* run(EbpfOptions& options, const IR::P4Program* program);
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_MIDEND_H_ */
