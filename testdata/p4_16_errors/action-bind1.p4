/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control c(inout bit<32> x) {
    action a(inout bit<32> b, bit<32> d) {
        b = d;
    }
    table t {
        actions = { a(x); }
        default_action = a();  // error: too few arguments
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);

top(c()) main;
