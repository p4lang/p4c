/*
 * SPDX-FileCopyrightText: 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C(out bit<64> v64) {
    action a() {
        v64 = (bit<64>)(int<64>)(int<1>)1w1;
    }

    table t {
        actions = { a; }
        default_action = a;
    }

    apply {
        t.apply();
    }
}

control proto(out bit<64> v64);
package top(proto p);

top(C()) main;
