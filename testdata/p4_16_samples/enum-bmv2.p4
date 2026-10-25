/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<32> b;
    bit<32> c;
}

enum Choice {
    First,
    Second
}

control compute(inout hdr h)
{
    apply {
        // Test enum lowering
        Choice c = Choice.First;

        if (c == Choice.Second)
            h.c = h.a;
        else
            h.c = h.b;
    }
}

#include "arith-inline-skeleton.p4"
