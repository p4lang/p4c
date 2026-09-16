/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_EBPF_MIDEND_H_
#define BACKENDS_EBPF_MIDEND_H_

#include "ebpfOptions.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4::EBPF {

class MidEnd {
 public:
    std::vector<DebugHook> hooks;
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    /// If p4c is run with option '--listMidendPasses', outStream is used for printing passes names.
    IR::Ptr<IR::ToplevelBlock> run(EbpfOptions &options, const IR::P4Program *program,
                                   std::ostream *outStream = nullptr);
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_MIDEND_H_ */
