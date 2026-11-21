/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern bit f(inout bit x, in bit y);
extern bit g(inout bit x);

header H { bit z; }

control c<T>(inout T t);
package top<T>(c<T> _c);

control my(inout H[2] s) {
    apply {
        bit a = 0;

        a = f(a, g(a));
        a = f(s[a].z, g(a));
        a = f(s[g(a)].z, a);
        a = g(a);
        a[0:0] = g(a[0:0]);
        s[a].z = g(a);
    }
}

top(my()) main;
