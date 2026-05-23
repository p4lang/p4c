/*
 * SPDX-FileCopyrightText: 2024 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header h_t {
    bit<8> f;
}

control C() {
    action bar() {
        h_t h;
        h.minSizeInBits();
    }

    table t {
        actions = { bar; }
        default_action = bar;
    }

    apply {
        t.apply();
    }
}

control proto();
package top(proto p);

top(C()) main;
