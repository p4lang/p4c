/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "comparison_headers.p4h"

header values_t {
    int<24> left24;
    int<24> right24;
    bit<40> left40;
    bit<40> right40;
    bit<56> left56;
    bit<56> right56;
    bit<8> padding;
}

struct array_headers_t {
    ethernet_t ethernet;
    values_t values;
}

parser ArrayParserImpl(packet_in pkt, out array_headers_t hdr, inout metadata_t meta,
                       in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.values);
        verify(hdr.ethernet.etherType != 0, error.BadEtherType);
        transition accept;
    }
}

control ArrayControlImpl(inout array_headers_t hdr, inout metadata_t meta,
                         in pna_main_input_metadata_t istd,
                         inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
            return;
        }
        bool matches = false;
        if (hdr.ethernet.etherType == 0x8918) {
            matches = hdr.values.left24 == hdr.values.right24;
        } else if (hdr.ethernet.etherType == 0x8a18) {
            matches = hdr.values.left24 != hdr.values.right24;
        } else if (hdr.ethernet.etherType == 0x8928) {
            matches = hdr.values.left40 == hdr.values.right40;
        } else if (hdr.ethernet.etherType == 0x8a28) {
            matches = hdr.values.left40 != hdr.values.right40;
        } else if (hdr.ethernet.etherType == 0x8938) {
            matches = hdr.values.left56 == hdr.values.right56;
        } else if (hdr.ethernet.etherType == 0x8a38) {
            matches = hdr.values.left56 != hdr.values.right56;
        } else if (hdr.ethernet.etherType == 0x8b18) {
            matches = hdr.values.left24 < hdr.values.right24;
        } else if (hdr.ethernet.etherType == 0x8c18) {
            matches = hdr.values.left24 <= hdr.values.right24;
        } else if (hdr.ethernet.etherType == 0x8d18) {
            matches = hdr.values.left24 > hdr.values.right24;
        } else if (hdr.ethernet.etherType == 0x8e18) {
            matches = hdr.values.left24 >= hdr.values.right24;
        }
        if (matches) {
            send_to_port(istd.input_port);
        } else {
            drop_packet();
        }
    }
}

control ArrayDeparserImpl(packet_out pkt, inout array_headers_t hdr, in metadata_t meta,
                          in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.values);
    }
}

PNA_NIC(ArrayParserImpl(), ArrayControlImpl(), ArrayDeparserImpl()) main;
