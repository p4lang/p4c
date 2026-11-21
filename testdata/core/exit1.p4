/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control ctrl() {
    apply {
        bit<32> a;
        bit<32> b;
        bit<32> c;

        a = 0;
        b = 1;
        c = 2;
        if (a == 0) {
            b = 2;
            exit;
            c = 3;
        } else {
            b = 3;
            exit;
            c = 4;
        }
        c = 5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
