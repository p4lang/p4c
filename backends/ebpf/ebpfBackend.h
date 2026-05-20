/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_EBPF_EBPFBACKEND_H_
#define BACKENDS_EBPF_EBPFBACKEND_H_

#include "ebpfObject.h"
#include "ebpfOptions.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"

namespace P4::EBPF {

void run_ebpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                      P4::ReferenceMap *refMap, P4::TypeMap *typeMap);

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_EBPFBACKEND_H_ */
