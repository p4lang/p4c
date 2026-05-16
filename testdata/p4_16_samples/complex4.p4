/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern E {
    E();
    bit<32> f(in bit<32> x);
}

control c(inout bit<32> r) {
    E() e;
    apply {
        r = e.f(4) + e.f(5);
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
