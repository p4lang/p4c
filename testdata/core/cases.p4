/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control ctrl() {
    action a() {}
    action b() {}
    action c() {}

    table t {
        actions = { a; b; c; }
        default_action = a;
    }

    apply {
        switch (t.apply().action_run) {
            a:
            b: { return; }
            c:
        }
    }
}