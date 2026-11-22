/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header H {
    bit<32> f;
}

control c() {
    packet_in() p;
    H h;

    apply {
        p.extract(h);
    }
}

control proto();
package top(proto _p);

top(c()) main;
