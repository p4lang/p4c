/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(inout bit<32> x) {
    apply {
        x = 10;
        if (x == 10) {
            x = x + 2;
            x = x - 6;
        } else {
            x = x << 2;
        }
    }
}

control n(inout bit<32> x);
package top(n _n);

top(c()) main;