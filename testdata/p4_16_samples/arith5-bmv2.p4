/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header hdr {
    int<32> a;
    bit<8>  b;
    int<64> c;
}

#include "arith-skeleton.p4"

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action shift()
    { h.h.c = (int<64>)(h.h.a >> h.h.b); sm.egress_spec = 0; }
    table t {
        actions = { shift; }
        const default_action = shift;
    }
    apply { t.apply(); }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
