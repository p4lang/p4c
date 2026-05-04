/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core.p4"

parser P(packet_in p, out bit<32> h) {
    state start {
        p.extract(h);  // error: not a header
        transition accept;
    }
}

parser Simple(packet_in p, out bit<32> h);

package top(Simple prs);
top(P()) main;
