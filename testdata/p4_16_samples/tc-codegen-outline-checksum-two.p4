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
struct metadata_t { bit<8> seen; bit<8> mark; bit<32> left; }

// Both outlined regions use the same checksum instance. Each helper would get
// its own copy, but they would also be copies of one program-level instance
// that the caller no longer owns, so a value written in either region would be
// lost. Outlining must be refused.
parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    InternetChecksum() ck;

    state start {
        pkt.extract(hdr.ethernet);
        meta.left = 0;
        ck.clear();
        ck.add({hdr.ethernet.kind});
        meta.seen = (bit<8>)ck.get();
        transition select(hdr.ethernet.kind) {
            0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract(hdr.ipv4);
        meta.left = (bit<32>)hdr.ipv4.fragment_offset;
        ck.clear();
        ck.add({hdr.ipv4.flags});
        meta.mark = (bit<8>)ck.get();
        transition select(hdr.ipv4.flags) {
            0: finish;
            default: validate;
        }
    }
    state validate {
        verify(meta.left <= 8191, error.ParserInvalidArgument);
        transition finish;
    }
    state finish {
        pkt.extract(hdr.tail);
        transition accept;
    }
}
control Ingress(inout headers_t hdr, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    apply { send_to_port((PortId_t)1); }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.tail);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
