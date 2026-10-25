/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core.p4"

header H {
    bit<32> field;
}

parser P(packet_in p, out H h) {
    state start {
        p.extract(h, 32);  // error: not a variable-sized header
        transition accept;
    }
}

parser Simple(packet_in p, out H h);
package top(Simple prs);
top(P()) main;
