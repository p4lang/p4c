/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<16> a;
}

control compute(inout hdr h)
{
    apply {
        hash(h.a, HashAlgorithm.crc16, 10w0, { 16w1 }, 10w4);
    }
}

#include "arith-inline-skeleton.p4"
