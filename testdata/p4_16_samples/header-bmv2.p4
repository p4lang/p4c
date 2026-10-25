/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> f;
}

control compute(inout hdr h) {
    apply {
        hdr tmp;
        tmp.f = h.f + 1;
        h.f = tmp.f;
    }
}

#include "arith-inline-skeleton.p4"
