// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include <core.p4>
#include <v1model.p4>

header h_t {
    int<8>  s;
    bit<8>  u;
    bit<16> r1;
    bit<16> r2;
}
struct headers { h_t h; }
struct meta {}

parser p(packet_in pk, out headers hdr, inout meta m, inout standard_metadata_t sm) {
    state start { pk.extract(hdr.h); transition accept; }
}
control vc(inout headers hdr, inout meta m) { apply {} }
control ig(inout headers hdr, inout meta m, inout standard_metadata_t sm) {
    apply {
        // Signed left operand, unsigned right operand: result is int<16>.
        int<16> a = hdr.h.s ++ hdr.h.u;
        // Unsigned left operand, signed right operand: result is bit<16>.
        bit<16> b = hdr.h.u ++ hdr.h.s;
        hdr.h.r1 = (bit<16>)a;
        hdr.h.r2 = b;
        sm.egress_spec = 0;
    }
}
control eg(inout headers hdr, inout meta m, inout standard_metadata_t sm) { apply {} }
control ck(inout headers hdr, inout meta m) { apply {} }
control dp(packet_out pk, in headers hdr) { apply { pk.emit(hdr.h); } }
V1Switch(p(), vc(), ig(), eg(), ck(), dp()) main;
