/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
// Two four-bit fields share a byte, and the second one is signed, so its
// stored value carries bits above the field.
header nibbles_t { bit<4> upper; int<4> lower; }
struct headers_t { ethernet_t ethernet; nibbles_t nibbles; }
struct metadata_t { bit<8> unused; }

// The header arguments are deliberately not called "hdr": the deparser writes
// them under the parser's names.
parser Parser(packet_in pkt, out headers_t pkt_hdrs, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(pkt_hdrs.ethernet);
        transition select(pkt_hdrs.ethernet.kind) {
            0x88b5: parse_nibbles;
            default: reject;
        }
    }
    state parse_nibbles {
        pkt.extract(pkt_hdrs.nibbles);
        transition accept;
    }
}
control Ingress(inout headers_t pkt_hdrs, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    apply { send_to_port((PortId_t)1); }
}
control Deparser(packet_out pkt, inout headers_t dep_hdrs, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply {
        dep_hdrs.nibbles.upper = 0xa;
        dep_hdrs.nibbles.lower = -1;
        pkt.emit(dep_hdrs.ethernet);
        pkt.emit(dep_hdrs.nibbles);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
