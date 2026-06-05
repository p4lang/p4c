/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control c(inout bit<32> x) {
    action a(in bit<32> arg) { x = arg; }

    apply {
        a(15);
    }
}

control proto(inout bit<32> arg);
package top(proto p);

top(c()) main;
