#ifndef _BACKENDS_EBPF_EBPFBACKEND_H_
#define _BACKENDS_EBPF_EBPFBACKEND_H_

#include "ebpfOptions.h"
#include "ebpfObject.h"
#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace EBPF {

void run_ebpf_backend(const EbpfOptions& options, const IR::ToplevelBlock* toplevel,
                      P4::ReferenceMap* refMap, const P4::TypeMap* typeMap);

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFBACKEND_H_ */
