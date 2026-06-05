/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
header H1 { bit<32> f; }
header H2 { bit<32> g; }

header_union U {
    H1 h1;
    H2 h2;
}

control ct();
package top(ct _ct);

control c() {
    apply {
        U u = { { 10 }, { 20 } };  // illegal to initialize unions
        u.setValid(); // no such method
    }
}
top(c()) main;