/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

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
