/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p1(out bit<2> w)(bit<2> a) {
    state start {
        w = 2;
        transition accept;
    }
}

parser p2(out bit<2> w)(bit<2> a) {
    p1(a) x;
    state start {
        x.apply(w);
        transition accept;
    }
}

parser simple(out bit<2> w);
package m(simple n);
m(p2(2w1)) main;
