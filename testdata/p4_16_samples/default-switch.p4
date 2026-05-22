/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control ctrl() {
    action a() {}
    action b() {}

    table t {
        actions = { a; b; }
        default_action = a;
    }

    apply {
        switch (t.apply().action_run) {
            b:
            default: { return; }
        }
    }
}
