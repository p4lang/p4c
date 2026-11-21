/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Test case for Issue #216

#include <core.p4>
#include <v1model.p4>

#include <core.p4>
#include <v1model.p4>

// List of all recognized headers
struct Headers {}

struct Metadata {}

parser P(packet_in b,
         out Headers p,
         inout Metadata meta,
         inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

// match-action pipeline section

control Ing(inout Headers headers,
            inout Metadata meta,
            inout standard_metadata_t standard_meta) {
    apply {}
}

control Eg(inout Headers hdrs,
           inout Metadata meta,
           inout standard_metadata_t standard_meta) {

    action update(in bool p, inout bit<64> val) {
        bit<32> _sub = val[31:0];
        _sub = p ? _sub : 32w1;
        val[31:0] = _sub;
    }

    apply {
        bit<64> res = 0;
        update(true, res);
    }
}

// deparser section
control DP(packet_out b, in Headers p) {
    apply {}
}

// Fillers
control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {}
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {}
}

// Instantiate the top-level V1 Model package.
V1Switch(P(),
         Verify(),
         Ing(),
         Eg(),
         Compute(),
         DP()) main;
