/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p1<T>(in T a) {
    state start {
        T w = a;
        transition accept;
    }
}

parser simple(in bit<2> a);

package m(simple n);

m(p1<bit<2>>()) main;
