/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Shifts where the amount and the value have different widths.
// BMv2 limits shift amounts to 8 bits, so the values are 4 bits wide.

#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<4>  a;
    int<4>  s;
    bit<8>  amt;
    bit<32> w;
    bit<8>  amt_w;
    bit<8>  path;
    bit<4>  a_shl;
    bit<4>  a_shr;
    int<4>  s_shr;
    bit<4>  padding;
    bit<32> w_shl;
    bit<32> w_shr;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
    apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply {
        // Truncating this amount to 4 bits would give a shift by 0.
        if (h.h.amt[3:0] == 0 && h.h.amt != 0) {
            h.h.path = 1;
        } else if ((h.h.a << h.h.amt) == 0 && h.h.a != 0) {
            h.h.path = 2;
        } else {
            h.h.path = 3;
        }
        if ((h.h.s >> h.h.amt) == -1) {
            h.h.path = h.h.path | 8w0x10;
        }
        if ((h.h.w << h.h.amt_w) == 0 && h.h.w != 0) {
            h.h.path = h.h.path | 8w0x20;
        }
        h.h.a_shl = h.h.a << h.h.amt;
        h.h.a_shr = h.h.a >> h.h.amt;
        h.h.s_shr = h.h.s >> h.h.amt;
        h.h.w_shl = h.h.w << h.h.amt_w;
        h.h.w_shr = h.h.w >> h.h.amt_w;
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
