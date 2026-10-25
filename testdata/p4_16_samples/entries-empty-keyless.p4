/*
 * Copyright 2025 Altera Corporation
 * SPDX-FileCopyrightText: 2025 Altera Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control c() {
    action a() {}
    table t {
        actions = { a; }
        default_action = a;
        entries = {}
    }
    apply {
        t.apply();
    }
}

control cc();
package top(cc p);

top(c()) main;
