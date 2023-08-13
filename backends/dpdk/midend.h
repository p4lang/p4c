/*
Copyright 2020 Intel Corp.

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

#ifndef BACKENDS_DPDK_MIDEND_H_
#define BACKENDS_DPDK_MIDEND_H_

#include <iosfwd>

#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "midend/convertEnums.h"

namespace DPDK {

class DpdkMidEnd : public PassManager {
 public:
    // These will be accurate when the mid-end completes evaluation
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    const IR::ToplevelBlock *toplevel = nullptr;
    P4::ConvertEnums::EnumMapping enumMap;

    // If p4c is run with option '--listMidendPasses', outStream is used for
    // printing passes names
    explicit DpdkMidEnd(CompilerOptions &options, std::ostream *outStream = nullptr);

    const IR::ToplevelBlock *process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel;
    }
};

}  // namespace DPDK

#endif /* BACKENDS_DPDK_MIDEND_H_ */
