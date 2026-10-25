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

parser p(out bit z) {
   p1(1w0) p1i;

   state start {
        p1i.apply(z);
        transition accept;
   }
}

parser simple(out bit z);
package m(simple n);

m(p()) main;
