/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
header tail_t { bit<8> value; }
struct headers_t { ethernet_t ethernet; tail_t tail; }
struct metadata_t { bit<8> seen; }

// The parser local is written before the outlined region and read inside it.
parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    bit<8> seen;

    state start {
        pkt.extract(hdr.ethernet);
        seen = hdr.ethernet.kind[7:0];
        transition select(hdr.ethernet.kind) {
            0x88b5: parse_tail;
            default: reject;
        }
    }
    state parse_tail {
        pkt.extract(hdr.tail);
        seen = seen + hdr.tail.value;
        transition select(hdr.tail.value) {
            0xff: reject;
            default: finish;
        }
    }
    state finish { meta.seen = seen; transition accept; }
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
        pkt.emit(hdr.tail);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
