/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header Header {
    bit<32> data;
}

parser p(packet_in pckt, out Header h) {
    state start {

        h.data = 0;
        h.data[3:0] = 2;    // skipped

        h.data = 7;
        h.data[7:0] = 4;    // skipped
        h.data[7:0] = 3;    // skipped
        h.data[15:0] = 8;   // stays
        h.data[31:16] = 5;  // stays

        transition accept;
    }
}

parser proto(packet_in pckt, out Header h);
package top(proto _p);

top(p()) main;
