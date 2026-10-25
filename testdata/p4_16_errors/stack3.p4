/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This test illustrates several limitations and incorrect uses of
 * header stacks, which must be declared with compile-time constant
 * sizes. */

#include <core.p4>

header h {}

parser p() {
    state start {
        transition accept;
    }
}

control c()(bit<32> x) {
    apply {
        h[4] s1;
        h[s1.size + 1] s2;
        h[x] s3;
   }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c(32w1)) main;
