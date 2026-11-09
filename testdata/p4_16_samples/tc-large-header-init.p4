/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

#define CHUNKS(F) \
    F(0) F(1) F(2) F(3) F(4) F(5) F(6) F(7) \
    F(8) F(9) F(10) F(11) F(12) F(13) F(14) F(15) \
    F(16) F(17) F(18) F(19) F(20) F(21) F(22) F(23) \
    F(24) F(25) F(26) F(27) F(28) F(29) F(30) F(31) \
    F(32) F(33) F(34) F(35) F(36) F(37) F(38) F(39) \
    F(40) F(41) F(42) F(43) F(44) F(45) F(46) F(47) \
    F(48) F(49) F(50) F(51) F(52) F(53) F(54) F(55) \
    F(56) F(57) F(58) F(59) F(60) F(61) F(62) F(63)

header ethernet_t { bit<48> dst; bit<48> src; bit<16> ether_type; }
header chunk_t { bit<64> a; bit<64> b; bit<64> c; bit<64> d; }
struct headers_t {
    ethernet_t ethernet;
#define FIELD(N) chunk_t chunk##N;
    CHUNKS(FIELD)
#undef FIELD
}
struct metadata_t { bit<8> unused; }

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            0x88b5: first;
            0x88b6: last;
            default: accept;
        }
    }
    state first { pkt.extract(hdr.chunk0); transition accept; }
    state last { pkt.extract(hdr.chunk63); transition accept; }
}
control Ingress(inout headers_t hdr, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) { drop_packet(); return; }
        send_to_port(istd.input_port);
    }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.chunk0);
        pkt.emit(hdr.chunk63);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
