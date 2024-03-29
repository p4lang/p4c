/*
Copyright 2013-present Barefoot Networks, Inc.
Copyright 2024 NVIDIA CORPORATION.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<16> key;
    bit<32> a;
    bit<32> b;
}

control compute(inout hdr h) {
    action add(bit<32> va, bit<32> vb = 0) {
        h.a = h.a + va;
        h.b = h.b + vb;
    }
    table t {
        key = { h.key : exact; }
        actions = { add; }
        const default_action = add(10, 0);
    }
    apply { t.apply(); }
}

#include "arith-inline-skeleton.p4"
