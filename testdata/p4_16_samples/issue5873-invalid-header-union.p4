/*
 * SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header h1_t { bit<8> f1; }
header h2_t { bit<8> f2; }

header_union hu_t { h1_t h1; h2_t h2; }

struct S { hu_t hu; }

control C(inout S s, in bit<8> k, out bool v) {
    action a() {
        S s_local = { ... };
        s = s_local;
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
        v = s.hu.isValid();
        t2.apply();
    }
}

control proto(inout S s, in bit<8> k, out bool v);
package top(proto p);

top(C()) main;
