/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "comparison_headers.p4h"

header signed_values_t {
    int<8> left8;
    int<8> right8;
    int<16> left16;
    int<16> right16;
    int<32> left32;
    int<32> right32;
    int<64> left64;
    int<64> right64;
    bit<8> padding;
}

struct signed_headers_t {
    ethernet_t ethernet;
    signed_values_t values;
}

parser SignedParserImpl(packet_in pkt, out signed_headers_t hdr, inout metadata_t meta,
                        in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.values);
        verify(hdr.ethernet.etherType != 0, error.BadEtherType);
        transition accept;
    }
}

control SignedControlImpl(inout signed_headers_t hdr, inout metadata_t meta,
                          in pna_main_input_metadata_t istd,
                          inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
            return;
        }
        bool matches = false;
        if (hdr.ethernet.etherType == 0x88b5) {
            matches = (hdr.values.left8 < hdr.values.right8) &&
                      (hdr.values.left16 < hdr.values.right16) &&
                      (hdr.values.left32 < hdr.values.right32) &&
                      (hdr.values.left64 < hdr.values.right64);
        } else if (hdr.ethernet.etherType == 0x88b6) {
            matches = (hdr.values.left8 <= hdr.values.right8) &&
                      (hdr.values.left16 <= hdr.values.right16) &&
                      (hdr.values.left32 <= hdr.values.right32) &&
                      (hdr.values.left64 <= hdr.values.right64);
        } else if (hdr.ethernet.etherType == 0x88b7) {
            matches = (hdr.values.left8 > hdr.values.right8) &&
                      (hdr.values.left16 > hdr.values.right16) &&
                      (hdr.values.left32 > hdr.values.right32) &&
                      (hdr.values.left64 > hdr.values.right64);
        } else if (hdr.ethernet.etherType == 0x88b8) {
            matches = (hdr.values.left8 >= hdr.values.right8) &&
                      (hdr.values.left16 >= hdr.values.right16) &&
                      (hdr.values.left32 >= hdr.values.right32) &&
                      (hdr.values.left64 >= hdr.values.right64);
        }
        if (matches) {
            send_to_port(istd.input_port);
        } else {
            drop_packet();
        }
    }
}

control SignedDeparserImpl(packet_out pkt, inout signed_headers_t hdr, in metadata_t meta,
                           in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.values);
    }
}

PNA_NIC(SignedParserImpl(), SignedControlImpl(), SignedDeparserImpl()) main;
