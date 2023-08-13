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

#ifndef BACKENDS_DPDK_BACKEND_H_
#define BACKENDS_DPDK_BACKEND_H_
#include <iosfwd>

#include "p4/config/v1/p4info.pb.h"
#include "p4/config/v1/p4types.pb.h"

namespace p4configv1 = ::p4::config::v1;
#undef setbit

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "options.h"

namespace DPDK {
class DpdkBackend {
    DpdkOptions &options;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const p4configv1::P4Info &p4info;
    const IR::DpdkAsmProgram *dpdk_program = nullptr;

 public:
    void convert(const IR::ToplevelBlock *tlb);
    DpdkBackend(DpdkOptions &options, P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                const p4configv1::P4Info &p4info)
        : options(options), refMap(refMap), typeMap(typeMap), p4info(p4info) {}
    void codegen(std::ostream &) const;
};

}  // namespace DPDK

#endif /* BACKENDS_DPDK_BACKEND_H_ */
