/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(inout bit<32> x) {
    action a(inout bit<32> b, bit<32> d) {
        b = d;
    }
    table t {
        actions = { a(x); }
        default_action = a(x, 0);
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);

top(c()) main;
