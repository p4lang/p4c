/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * SPDX-FileCopyrightText: 2024 NVIDIA CORPORATION
 *
 * SPDX-License-Identifier: Apache-2.0
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
