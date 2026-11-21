/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
struct S {
    bit<32> f;
}

control c(inout S data) {
    apply {
        data.f = 0;
    }
}

control caller() {
    S data;
    c() cinst;

    apply {
        data.f = 0;
        cinst.apply(data);
    }
}

control none();
package top(none n);

top(caller()) main;
