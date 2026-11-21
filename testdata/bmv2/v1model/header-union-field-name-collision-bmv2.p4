/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>
@command_line("--loopsUnroll")

header H1 { bit<8> a; }
header H2 { bit<16> b; }
header_union HU { H1 x; H2 y; }

// The packet headers and user metadata deliberately share union field names.
// p4test must flatten the fields independently for these two structs.
struct A {
    HU u;
    HU[2] hus;
}

struct B {
    HU u;
    HU[2] hus;
}

parser ParserImpl(packet_in pkt, out A hdr, inout B meta,
                  inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.u.x);
        pkt.extract(hdr.hus[0].x);
        pkt.extract(hdr.hus[1].y);
        pkt.extract(meta.u.x);
        pkt.extract(meta.hus[0].x);
        pkt.extract(meta.hus[1].y);
        transition accept;
    }
}

control VerifyChecksumImpl(inout A hdr, inout B meta) { apply {} }
control ComputeChecksumImpl(inout A hdr, inout B meta) { apply {} }

control IngressImpl(inout A hdr, inout B meta, inout standard_metadata_t sm) {
    action rewrite(bit<8> a, bit<8> b, bit<16> c) {
        hdr.u.x.a = a + meta.u.x.a;
        hdr.hus[0].x.a = b + meta.hus[0].x.a;
        hdr.hus[1].y.b = c + meta.hus[1].y.b;
    }
    table t {
        key = {
            hdr.u.x.a: exact;
            hdr.hus[0].x.a: exact;
            hdr.hus[1].y.b: exact;
            meta.u.x.a: exact;
            meta.hus[0].x.a: exact;
            meta.hus[1].y.b: exact;
        }
        actions = { rewrite; NoAction; }
        const default_action = NoAction();
    }
    apply {
        t.apply();
        sm.egress_spec = 1;
    }
}

control EgressImpl(inout A hdr, inout B meta, inout standard_metadata_t sm) {
    apply {}
}

control DeparserImpl(packet_out pkt, in A hdr) {
    apply {
        pkt.emit(hdr.u);
        pkt.emit(hdr.hus);
    }
}

V1Switch(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(),
         ComputeChecksumImpl(), DeparserImpl()) main;
