/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

header h_t {
    bit<8> f;
}

struct my_packet {
    h_t[10] h;
}

parser MyParser(packet_in b, out my_packet p) {
    state start {
        b.extract(p.h);
        transition accept;
    }
}