/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct S {
    bit a;
    bit b;
}

control c(out bit b) {
    apply {
        S s = { 0, 1 };
        s = { s.b, s.a };
        b = s.a;
    }
}

control e<T>(out T t);
package top<T>(e<T> e);

top(c()) main;
