// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

// An aligned maximum must not hide unaligned fixed fields.
#include <core.p4>
#include <v1model.p4>
header variable_t { bit<1> tag; varbit<63> data; }
struct headers_t { variable_t variable; }
struct metadata_t {}
parser ParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta,
                  inout standard_metadata_t sm) {
    state start { pkt.extract(hdr.variable, 7); transition accept; }
}
control VerifyImpl(inout headers_t h, inout metadata_t m) { apply {} }
control IngressImpl(inout headers_t h, inout metadata_t m,
                    inout standard_metadata_t sm) { apply { sm.egress_spec = 0; } }
control EgressImpl(inout headers_t h, inout metadata_t m,
                   inout standard_metadata_t sm) { apply {} }
control ComputeImpl(inout headers_t h, inout metadata_t m) { apply {} }
control DeparserImpl(packet_out p, in headers_t h) { apply { p.emit(h.variable); } }
V1Switch(ParserImpl(), VerifyImpl(), IngressImpl(), EgressImpl(), ComputeImpl(), DeparserImpl()) main;
