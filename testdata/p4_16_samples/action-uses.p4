/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c() {
    action a() {}
    action b() {}
    table t1 {
        actions = { a; b; }
        default_action = a;
    }
    table t2 {
        actions = { a; }
        default_action = a;
    }

    apply {
        t1.apply();
        t2.apply();
    }
}

control empty();
package top(empty e);
top(c()) main;
