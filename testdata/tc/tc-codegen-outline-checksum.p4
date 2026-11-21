/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
header tail_t { bit<8> value; }
header last_t { bit<8> mark; }
struct headers_t { ethernet_t ethernet; tail_t tail; last_t last; }
struct metadata_t { bit<8> seen; }

// The checksum is a parser-local extern instance and the outlined region uses
// it. The instance is registered while the normal parser body is emitted, which
// happens after the helper has been built, so the helper cannot reach it.
parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    InternetChecksum() ck;

    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.kind) {
            0x88b5: parse_checksum;
            0x88b6: parse_last;
            default: reject;
        }
    }
    state parse_checksum {
        pkt.extract(hdr.tail);
        ck.clear();
        ck.add({hdr.tail.value});
        meta.seen = (bit<8>)ck.get();
        transition parse_last;
    }
    state parse_last {
        pkt.extract(hdr.last);
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
        pkt.emit(hdr.tail);
        pkt.emit(hdr.last);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
