/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(out bit<32> x) {
    action a1() {}
    action a2() {}
    action a3() {}

    table t {
        actions = { a1; a2; }
        default_action = a1;
    }

    apply {
        switch (t.apply().action_run) {
            a1:
            a2: { return; }
            default: { return; }
        }
    }
}

control d(out bit<32> x) {
    c() cinst;

    apply {
        cinst.apply(x);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);

top(d()) main;
