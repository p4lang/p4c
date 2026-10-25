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
}

control compute(inout hdr h) {
    action a() { h.b = h.a; }
    table t {
        key = { h.a + h.a : exact @name("e"); }
        actions = { a; NoAction; }
        default_action = NoAction;
    }
    apply {
        t.apply();
    }
}

#include "arith-inline-skeleton.p4"
