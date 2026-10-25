/*
 * SPDX-FileCopyrightText: 2024 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
@command_line("--loopsUnroll")

parser p(packet_in packet, out bit<8> f, out bit<4> g) {
    state start {
        g = f[3:0];
        transition accept;
    }
}

parser simple(packet_in packet, out bit<8> f, out bit<4> g);
package top(simple e);
top(p()) main;
