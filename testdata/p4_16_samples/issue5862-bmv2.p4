/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header Result { bit<8> flags; }
header H { bit<8> x; }
header_union U { H a; H b; }
struct Headers { Result result; U u; }
struct Meta {}

parser P(packet_in packet, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        packet.extract(h.result);
        packet.extract(h.u.a);
        transition accept;
    }
}

control I(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        // The write may clobber a.x, but must not change either validity bit.
        h.u.b.x = 2;
        h.result.flags = 0;
        if (h.u.a.isValid()) { h.result.flags = h.result.flags | 1; }
        if (h.u.b.isValid()) { h.result.flags = h.result.flags | 2; }
        sm.egress_spec = 0;
    }
}

control E(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }
control Verify(inout Headers h, inout Meta m) { apply {} }
control Update(inout Headers h, inout Meta m) { apply {} }
control D(packet_out packet, in Headers h) { apply { packet.emit(h.result); } }
V1Switch(P(), Verify(), I(), E(), Update(), D()) main;
