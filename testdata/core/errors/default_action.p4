/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control c() {
    action a() {}
    action b() {}
    table t {
        actions = { a; }
        default_action = b;  // not in the list of actions
    }
    apply {}
}