/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f<T>(in T data);

struct S {
    bit<32> f;
}

struct S1 {
    bit<32> f;
    bit<32> g;
}

action a() {
    S x;
    S1 y;

    f<S>( { 32w0 } );
    f<S>(x);
    f<S1>(y);
    f<S1>( { 32w0, 32w1 } );
}
