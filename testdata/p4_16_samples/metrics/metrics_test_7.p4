/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct T { bit f; }

struct S {
    tuple<T, T> f1;
    T f2;
    bit z;
}

struct tuple_0 {
    T field;
    T field_0;
}

extern void f<T>(in T data);

control c(inout bit r) {
    apply {
        S s = { { {0}, {1} }, {0}, 1 };
        f(s.f1);
        f<tuple_0>({{0},{1}});
        r = s.f2.f & s.z;
    }
}

control simple(inout bit r);
package top(simple e);
top(c()) main;

