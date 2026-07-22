// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "ebpfModel.h"

namespace P4::EBPF {

cstring EBPFModel::reservedPrefix = "ebpf_"_cs;
EBPFModel EBPFModel::instance;

}  // namespace P4::EBPF
