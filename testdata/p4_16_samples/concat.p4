/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control proto(out bit<32> x);
package top(proto _c);

control c(out bit<32> x) {
    apply {
        bit<8> a = 0xF;
        bit<16> b = 0xF;
        x = (a ++ b ++ a) + (b ++ (a ++ a));
    }
}

top(c()) main;
