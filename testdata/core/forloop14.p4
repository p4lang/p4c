/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// A for-in loop can run zero times, so the value from before the loop must survive.

#include <core.p4>
control generic<M>(inout M m);
package top<M>(generic<M> c);

header t1 {
    bit<32>     init;
    bit<32>     limit;
    bit<32>     output;
    bit<32>     field;
}

struct headers_t {
    t1          head;
}

control c(inout headers_t hdrs) {
    apply {
        bit<32> x = hdrs.head.init;
        for (bit<32> i in hdrs.head.limit .. 32w3) {
            x = 32w7;
        }
        hdrs.head.output = x;
        bit<32> y = hdrs.head.init;
        for (bit<32> j in 32w5 .. 32w3) {
            y = 32w8;
        }
        hdrs.head.field = y;
    }
}

top(c()) main;
