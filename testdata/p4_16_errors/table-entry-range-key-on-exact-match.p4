/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// An exact key is compared in full, so a range in one of its entries cannot be
// matched; the back end would keep only the first value of each range block.

#include <core.p4>

struct Headers {
    bit<8> a;
}

control ingress(inout Headers h) {
    action a1(bit<8> v) { h.a = v; }

    table t1 {
        key = { h.a : exact; }
        actions = { a1; NoAction; }
        const entries = {
            1..8 : a1(1);
        }
    }

    apply {}
}
