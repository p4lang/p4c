/*
 * Copyright 2018 Barefoot Networks, Inc.
 * SPDX-FileCopyrightText: 2018 Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

extern widget<T> { }

extern widget<T> createWidget<T, U>(U a, T b);

header hdr_t {
    bit<16>     f1;
    bit<16>     f2;
    bit<16>     f3;
}

control c1<T>(inout hdr_t hdr)(widget<T> w) { apply {} }

control c2(inout hdr_t hdr) {
    c1<bit<16>>(createWidget(hdr.f1, hdr.f2)) c;  // factory args must be constants
    apply {
        c.apply(hdr);
    }
}
