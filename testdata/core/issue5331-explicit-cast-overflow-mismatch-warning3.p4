/*
 * SPDX-FileCopyrightText: 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void __e(in bit<4> x);
control C() {
    action a() {
        __e(72);
    }

    table t {
        actions = { a; }
        default_action = a;
    }

    apply {
        t.apply();
    }
}

control proto();
package top(proto p);

top(C()) main;
