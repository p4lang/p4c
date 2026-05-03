/*
 * Copyright 2020 Intel Corp.
 * SPDX-FileCopyrightText: 2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_DPDK_MIDEND_H_
#define BACKENDS_DPDK_MIDEND_H_

#include "frontends/common/options.h"
#include "ir/ir.h"
#include "midend/convertEnums.h"

namespace P4::DPDK {

class DpdkMidEnd : public PassManager {
 public:
    /// These will be accurate when the mid-end completes evaluation.
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    const IR::ToplevelBlock *toplevel = nullptr;
    P4::ConvertEnums::EnumMapping enumMap;

    /// If p4c is run with option '--listMidendPasses', outStream is used for
    /// printing passes names.
    explicit DpdkMidEnd(CompilerOptions &options, std::ostream *outStream = nullptr);

    const IR::ToplevelBlock *process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel;
    }
};

}  // namespace P4::DPDK

#endif /* BACKENDS_DPDK_MIDEND_H_ */
