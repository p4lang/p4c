/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
header flags_t { bit<3> flags; bit<13> fragment_offset; }
header tail_t { bit<8> value; }
struct headers_t { ethernet_t ethernet; flags_t ipv4; tail_t tail; }
struct metadata_t { bit<32> left; bit<32> leftover; }

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        meta.left = 0;
        meta.leftover = 1;
        transition select(hdr.ethernet.kind) {
            0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract(hdr.ipv4);
        meta.left = (bit<32>)hdr.ipv4.fragment_offset;
        transition select(hdr.ipv4.flags) {
            0: finish;
            default: validate;
        }
    }
    state validate {
        verify(meta.left == 0, error.ParserInvalidArgument);
        meta.leftover = meta.leftover + 1;
        transition finish;
    }
    state finish { pkt.extract(hdr.tail); transition accept; }
}
control Ingress(inout headers_t hdr, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) { drop_packet(); return; }
        if (meta.leftover == 0) { drop_packet(); return; }
        send_to_port((PortId_t)1);
    }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply { pkt.emit(hdr.ethernet); pkt.emit(hdr.ipv4); pkt.emit(hdr.tail); }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
