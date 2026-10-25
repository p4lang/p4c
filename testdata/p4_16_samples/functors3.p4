/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p1(out bit z1)(bit b1) {
    state start {
        z1 = b1;
        transition accept;
    }
}

parser p(out bit z)(bit b, bit c) {
   p1(b) p1i;

   state start {
        p1i.apply(z);
        z = z & b & c;
        transition accept;
   }
}

const bit bv = 1w0;

parser simple(out bit z);
package m(simple n);

m(p(bv, 1w1)) main;
