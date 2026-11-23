/*
 * SPDX-FileCopyrightText: 2026 anant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

control C(inout bit<8> h) {
    action a(inout bit<8> x, bit<8> y) { x = y; }

    // The property comes before the action list that binds x.
    table t {
        my_action = a(h, 3);
        actions = { a(h); }
    }

    apply { t.apply(); }
}

control CT(inout bit<8> h);
package Top(CT c);
Top(C()) main;
