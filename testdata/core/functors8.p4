/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

extern e<T> {
    e();
    T get();
}

parser p1<T>(out T a) {
    e<T>() ei;
    state start {
        a = ei.get();
        transition accept;
    }
}

parser simple(out bit<2> a);

package m(simple n);

m(p1()) main;
