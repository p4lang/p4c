/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control c(inout bit<32> x) {
    bit<32> y;
    action a(inout bit<32> b, bit<32> d) {
        b = d;
    }
    table t {
        actions = { a(x); }
        default_action = a(y, 0);  // error: different bound argument
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);

top(c()) main;
