/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

header H {
    varbit<120> x;
}

parser p(packet_in pkt) {
    H h;
    state start {
        h = pkt.lookahead<H>();
        transition accept;
    }
}
