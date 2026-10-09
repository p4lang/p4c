/*
 * SPDX-FileCopyrightText: 2025 Nvidia Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

control ingress(inout Headers h) {
    action a1(bit<8> v) { h.a = v; }

    table t_exact {
        key = { h.b : exact; }
        actions = { a1; NoAction; }
        const entries = {
            0x02 : a1(1);
            2    : a1(2);
        }
    }

    apply {
        t_exact.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);

top(ingress()) main;
