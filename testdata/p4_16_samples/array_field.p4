/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H { bit z; }

extern bit<32> f(inout bit x, in bit b);

control c(out H[2] h);
package top(c _c);

control my(out H[2] s) {
    apply {
        bit<32> a = 0;
        s[a].z = 1;
        s[a+1].z = 0;
        a = f(s[a].z, 0);
        a = f(s[a].z, 1);
    }
}

top(my()) main;
