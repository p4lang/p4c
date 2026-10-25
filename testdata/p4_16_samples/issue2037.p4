/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action a() {}
control c() {
    table t {
        actions = { .a; }
        default_action = a;
    }
    apply {}
}
