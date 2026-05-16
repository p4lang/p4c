/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f<T>(in T data);
control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x_0;
    apply {
        x_0 = { 32w10, false };
        f<tuple<bit<32>, bool>>(x_0);
    }
}

top(c()) main;
