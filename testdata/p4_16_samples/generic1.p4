/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern Generic<T> {
    Generic(T sz);
    R get<R>();
    R get1<R, S>(in S value, in R data);
}

extern void f<T>(in T arg);

control c<T>()(T size) {
    Generic<T>(size) x;
    apply {
        bit<32> a = x.get<bit<32>>();
        bit<5> b = x.get1(10w0, 5w0);
        f(b);
    }
}

control caller() {
    c(8w9) cinst;
    apply {
        cinst.apply();
    }
}

control s();
package p(s parg);
p(caller()) main;
