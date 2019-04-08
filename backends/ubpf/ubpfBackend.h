#ifndef P4C_UBPFBACKEND_H
#define P4C_UBPFBACKEND_H

#include "backends/ebpf/ebpfOptions.h"
#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace UBPF {
    void run_ubpf_backend(const EbpfOptions& options, const IR::ToplevelBlock* toplevel,
                          P4::ReferenceMap* refMap, P4::TypeMap* typeMap);
} // namespace UBPF

#endif //P4C_UBPFBACKEND_H
