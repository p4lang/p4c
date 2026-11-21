/*
 * SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header h1_t { bit<8> f1; }

struct S { h1_t h; }

control C(inout S s, in bit<8> k, out bool v) {
    action a() {
        s.h = (h1_t){#};
    }

    table t1 {
        actions = { a; }
        default_action = a;
    }

    table t2 {
        key = { k : exact; }
        actions = { a; }
        default_action = a;
    }

    apply {
        t1.apply();
        v = s.h.isValid();
        t2.apply();
    }
}

control proto(inout S s, in bit<8> k, out bool v);
package top(proto p);

top(C()) main;
