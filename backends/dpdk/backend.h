/*
 * Copyright 2020 Intel Corp.
 * SPDX-FileCopyrightText: 2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_DPDK_BACKEND_H_
#define BACKENDS_DPDK_BACKEND_H_
#include "p4/config/v1/p4info.pb.h"

namespace p4configv1 = ::p4::config::v1;
#undef setbit

#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/json.h"
#include "options.h"

namespace P4::DPDK {
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

}  // namespace P4::DPDK

#endif /* BACKENDS_DPDK_BACKEND_H_ */
