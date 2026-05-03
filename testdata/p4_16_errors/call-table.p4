/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c() {
    action a() {}
    table t {
        actions = { a(); }
        default_action = a();
    }
    action b() {
        t.apply();  // cannot invoke table from action
    }
    apply {}
}
