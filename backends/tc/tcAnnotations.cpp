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

namespace TC {

const cstring ParseTCAnnotations::defaultHit = "default_hit";
const cstring ParseTCAnnotations::defaultHitConst = "default_hit_const";
const cstring ParseTCAnnotations::tcType = "tc_type";
const cstring ParseTCAnnotations::numMask = "nummask";
const cstring ParseTCAnnotations::tcMayOverride = "tc_may_override";
const cstring ParseTCAnnotations::tc_md_write = "tc_md_write";
const cstring ParseTCAnnotations::tc_md_read = "tc_md_read";
const cstring ParseTCAnnotations::tc_md_exec = "tc_md_exec";
const cstring ParseTCAnnotations::tc_ContrlPath = "tc_ContrlPath";
const cstring ParseTCAnnotations::tc_key = "tc_key";
const cstring ParseTCAnnotations::tc_data = "tc_data";
const cstring ParseTCAnnotations::tc_data_scalar = "tc_data_scalar";
const cstring ParseTCAnnotations::tc_init_val = "tc_init_val";
const cstring ParseTCAnnotations::tc_numel = "tc_numel";
const cstring ParseTCAnnotations::tc_acl = "tc_acl";

}  // namespace TC
