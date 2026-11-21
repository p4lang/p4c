/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
@command_line("--loopsUnroll")

header h1 { bit<8> a; }
header h2 { bit<16> b; }
header_union HU { h1 x; h2 y; }

struct A {
    HU u;
    HU[2] hus;
}

struct B {
    HU u;
    HU[2] hus;
}

control C(inout A a, inout B b);
package top(C c);

control c(inout A a, inout B b) {
    apply {
        a.u.x.setValid();
        b.u.x.setValid();
        a.hus[0].x.setValid();
        b.hus[0].x.setValid();
    }
}

top(c()) main;
