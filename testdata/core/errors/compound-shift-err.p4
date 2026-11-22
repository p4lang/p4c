/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);

@test_keep_opassign

header t1 {
    bit<32> f1;
    bool    b1;
}

struct headers_t {
    t1 head;
}

control c(inout headers_t hdrs) {
    apply {
        hdrs.head.f1 <<= hdrs.head.b1;
        hdrs.head.f1 >>= hdrs.head.b1;
    }
}

top(c()) main;
