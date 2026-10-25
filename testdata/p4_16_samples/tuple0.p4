/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f(in tuple<bit<32>, bool> data);

control proto();
package top(proto _p);

control c() {
    tuple<bit<32>, bool> x = { 10, false };
    apply {
        f(x);
        f({20, true});
    }
}

top(c()) main;
