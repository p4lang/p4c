/*
 * SPDX-FileCopyrightText: 2026 anant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p(packet_in packet) {
    value_set<bit<16>>(1) vs;
    bit<8> x = 0;
    state start {
        transition select(x, vs) {
            (1, 1): nested;
            default: reject;
        }
    }
    state nested {
        transition select(x, { x, vs }) {
            default: accept;
        }
    }
}

parser simple(packet_in packet);
package top(simple e);
top(p()) main;
