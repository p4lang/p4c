/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

struct S {
    H h1;
    H h2;
    bit<32> c;
}

parser p() {
    state start {
        S s;
        s.c = 0;
        transition accept;
    }
}

parser empty();
package top(empty e);
top(p()) main;
