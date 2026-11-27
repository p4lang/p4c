/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
header flags_t { bit<3> flags; bit<13> fragment_offset; }
header pair_t { bit<8> high; bit<8> low; }
header wide_t { bit<128> value; }
struct headers_t { ethernet_t ethernet; flags_t ipv4; pair_t pair; wide_t wide; }
struct metadata_t { bit<8> seen; bit<16> sum; }

// Every add below is a multiple of 16 bits, which the checksum extern requires:
// one 16-bit field, two fields that fill one word between them (three and
// thirteen bits), two eight-bit fields, and one field wider than 64 bits. The
// two-field adds are where the emitter accumulates bits across fields rather
// than adding each one on its own.
parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    InternetChecksum() ck;

    state start {
        pkt.extract(hdr.ethernet);
        ck.clear();
        ck.add({hdr.ethernet.kind});
        transition select(hdr.ethernet.kind) {
            0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract(hdr.ipv4);
        ck.add({hdr.ipv4.flags, hdr.ipv4.fragment_offset});
        transition parse_pair;
    }
    state parse_pair {
        pkt.extract(hdr.pair);
        ck.add({hdr.pair.high, hdr.pair.low});
        transition parse_wide;
    }
    state parse_wide {
        pkt.extract(hdr.wide);
        ck.add({hdr.wide.value});
        meta.sum = ck.get();
        meta.seen = 1;
        transition accept;
    }
}
control Ingress(inout headers_t hdr, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) { drop_packet(); return; }
        if (meta.seen == 0) { drop_packet(); return; }
        send_to_port((PortId_t)1);
    }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.pair);
        pkt.emit(hdr.wide);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
