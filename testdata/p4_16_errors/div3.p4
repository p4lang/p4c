/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control c(inout bit<8> a) {
    bit<8> b = 3;
    apply {
        a = a / b;  // not a compile-time constant
    }
}

control proto(inout bit<8> _a);
package top(proto _p);
top(c()) main;
