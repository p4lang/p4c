/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern bit<32> f(in bit<32> x);

control c(inout bit<32> r) {
    apply {
        r = f(f(5));
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
