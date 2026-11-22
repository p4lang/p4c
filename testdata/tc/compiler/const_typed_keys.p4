/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

header ethernet_t { bit<48> dstAddr; bit<48> srcAddr; bit<16> etherType; }
header ipv4_t { bit<32> srcAddr; bit<32> dstAddr; }
header ipv6_t { bit<128> dstAddr; }
struct headers_t { ethernet_t ethernet; ipv4_t ipv4; ipv6_t ipv6; }
struct metadata_t {}

parser ParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta,
                  in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.ipv4);
        pkt.extract(hdr.ipv6);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout metadata_t meta,
                        in pna_main_input_metadata_t istd,
                        inout pna_main_output_metadata_t ostd) {
    action send(@tc_type("dev") PortId_t port) { send_to_port(port); }
    action drop() { drop_packet(); }

    // iproute2 reads ipv4 and ipv6 keys as addresses with optional hex masks.
    table typed_keys {
        key = {
            hdr.ipv4.dstAddr : ternary @tc_type("ipv4");
            hdr.ipv6.dstAddr : ternary @tc_type("ipv6");
        }
        actions = { send; drop; }
        const entries = {
            (32w0x0a000001, 128w0x20010db8000000000000000000000001) : drop();
            (32w0x0a000000 &&& 32w0xff000000,
             128w0x20010db8000000000000000000000000 &&& 128w0x00ffffff000000000000000000000000) : drop();
        }
        const default_action = drop();
    }

    apply { typed_keys.apply(); }
}

control DeparserImpl(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                     in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv6);
    }
}

PNA_NIC(ParserImpl(), MainControlImpl(), DeparserImpl()) main;
