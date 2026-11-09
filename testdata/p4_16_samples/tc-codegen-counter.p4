/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
struct headers_t { ethernet_t ethernet; }
struct metadata_t { bit<8> unused; }

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    state start { pkt.extract(hdr.ethernet); transition accept; }
}
control Ingress(inout headers_t hdr, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    Counter<bit<64>, bit<32>>(8, PNA_CounterType_t.PACKETS) counters;
    apply {
        if (istd.parser_error != error.NoError) { drop_packet(); return; }
        counters.count(0);
        counters.count(7);
        counters.count((bit<32>)(hdr.ethernet.kind & 7));
        if (hdr.ethernet.kind != 0) {
            counters.count((bit<32>)hdr.ethernet.kind & 7);
            counters.count((bit<32>)hdr.ethernet.kind >> 1);
        }
        send_to_port((PortId_t)1);
    }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply { pkt.emit(hdr.ethernet); }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
