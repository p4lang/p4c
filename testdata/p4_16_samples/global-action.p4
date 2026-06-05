/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action Global() {}

control c() {
    table t {
        actions = { Global; }
        default_action = Global;
    }

    apply {
        t.apply();
    }
}

control none();
package top(none n);
top(c()) main;
