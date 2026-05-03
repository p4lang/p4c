/*
 * Copyright 2019 Orange
 * SPDX-FileCopyrightText: 2019 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_UBPF_UBPFBACKEND_H_
#define BACKENDS_UBPF_UBPFBACKEND_H_

#include "backends/ebpf/ebpfOptions.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"

namespace P4::UBPF {

void run_ubpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                      P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
std::string extract_file_name(const std::string &fullPath);

}  // namespace P4::UBPF

#endif /* BACKENDS_UBPF_UBPFBACKEND_H_ */
