// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include <core.p4>
#include <pna.p4>

// Fixed-length packet headers cannot be padded to a byte boundary.
header variable_t {
    bit<1> tag;
}

struct headers_t {
    variable_t variable;
}

struct empty_t {}

parser MainParserImpl(packet_in packet,
                      out headers_t hdr,
                      inout empty_t user_meta,
                      in pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract(hdr.variable);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr,
                       inout empty_t user_meta,
                       in pna_pre_input_metadata_t istd,
                       inout pna_pre_output_metadata_t ostd) {
    apply {}
}

control MainControlImpl(inout headers_t hdr,
                        inout empty_t user_meta,
                        in pna_main_input_metadata_t istd,
                        inout pna_main_output_metadata_t ostd) {
    apply {}
}

control MainDeparserImpl(packet_out packet,
                         in headers_t hdr,
                         in empty_t user_meta,
                         in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit(hdr.variable);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
