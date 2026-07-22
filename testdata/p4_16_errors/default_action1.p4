/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
action b() {}

control c() {
    action a() {}
    action b() {}
    table t {
        actions = { a; b; }
        default_action = .b();  // not the same b
    }
    apply {}
}