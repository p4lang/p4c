/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p(packet_in packet) {
    state start {
        transition select(packet.lookahead<bit<8>>()) {
            8w0x11: accept;
            8w0x11: reject;
        }
    }
}

parser simple(packet_in packet);
package top(simple prs);
top(p()) main;
