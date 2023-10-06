/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef BACKENDS_EBPF_MIDEND_H_
#define BACKENDS_EBPF_MIDEND_H_

#include "ebpfOptions.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace EBPF {

class MidEnd {
 public:
    std::vector<DebugHook> hooks;
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    // If p4c is run with option '--listMidendPasses', outStream is used for printing passes names
    const IR::ToplevelBlock *run(EbpfOptions &options, const IR::P4Program *program,
                                 std::ostream *outStream = nullptr);
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_MIDEND_H_ */
