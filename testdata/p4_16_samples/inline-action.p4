/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control p(inout bit bt) {
    action a(inout bit y0)
    { y0 = y0 | 1w1; }

    action b() {
        a(bt);
        a(bt);
    }

    table t {
        actions = { b; }
        default_action = b;
    }

    apply {
        t.apply();
    }
}

control simple<T>(inout T arg);
package m<T>(simple<T> pipe);

m(p()) main;
