// Copyright (C) 2023 Intel Corporation
// SPDX-FileCopyrightText: 2023 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "tcAnnotations.h"

namespace P4::TC {

using namespace P4::literals;

const cstring ParseTCAnnotations::defaultHit = "default_hit"_cs;
const cstring ParseTCAnnotations::defaultHitConst = "default_hit_const"_cs;
const cstring ParseTCAnnotations::tcType = "tc_type"_cs;
const cstring ParseTCAnnotations::numMask = "nummask"_cs;
const cstring ParseTCAnnotations::tcMayOverride = "tc_may_override"_cs;
const cstring ParseTCAnnotations::tc_md_write = "tc_md_write"_cs;
const cstring ParseTCAnnotations::tc_md_read = "tc_md_read"_cs;
const cstring ParseTCAnnotations::tc_md_exec = "tc_md_exec"_cs;
const cstring ParseTCAnnotations::tc_ControlPath = "tc_ControlPath"_cs;
const cstring ParseTCAnnotations::tc_key = "tc_key"_cs;
const cstring ParseTCAnnotations::tc_data = "tc_data"_cs;
const cstring ParseTCAnnotations::tc_data_scalar = "tc_data_scalar"_cs;
const cstring ParseTCAnnotations::tc_init_val = "tc_init_val"_cs;
const cstring ParseTCAnnotations::tc_numel = "tc_numel"_cs;
const cstring ParseTCAnnotations::tc_acl = "tc_acl"_cs;

}  // namespace P4::TC
