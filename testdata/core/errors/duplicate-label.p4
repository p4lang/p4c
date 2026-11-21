/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(out bit arun) {
    action a() {}
    table t {
        actions = { a; }
        default_action = a;
    }
    apply {
        switch (t.apply().action_run) {
            a: { arun = 1; }
            a: { arun = 1; }  // duplicate label
        }
    }
}

control proto(out bit run);
package top(proto _p);

top(c()) main;
