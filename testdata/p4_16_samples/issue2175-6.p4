/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(inout bit<32> y) {
    apply {
    }
}

control t(inout bit<32> b) {
    c() c1;

    apply {
        c1.apply(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);

top(t()) main;
