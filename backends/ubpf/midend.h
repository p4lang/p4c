/*
 * Copyright 2019 Orange
 * SPDX-FileCopyrightText: 2019 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_UBPF_MIDEND_H_
#define BACKENDS_UBPF_MIDEND_H_

#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/midend.h"
#include "ir/ir.h"

namespace P4::UBPF {

class MidEnd : public EBPF::MidEnd {
 public:
    MidEnd() : EBPF::MidEnd() {}
    const IR::ToplevelBlock *run(EbpfOptions &options, const IR::P4Program *program,
                                 std::ostream *outStream = nullptr);
};

}  // namespace P4::UBPF

#endif /* BACKENDS_UBPF_MIDEND_H_ */
