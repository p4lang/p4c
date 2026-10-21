/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "comparison_headers.p4h"

control ControlImpl(inout headers_t hdr, inout metadata_t meta,
                    in pna_main_input_metadata_t istd,
                    inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
            return;
        }
        bool matches = false;
        if (hdr.ethernet.etherType == 0x88b5) {
            matches = hdr.ethernet.srcAddr == hdr.ethernet.dstAddr;
        } else if (hdr.ethernet.etherType == 0x88b6) {
            matches = hdr.ethernet.srcAddr != hdr.ethernet.dstAddr;
        } else if (hdr.ethernet.etherType == 0x88b7) {
            matches = hdr.ethernet.srcAddr == 48w0x021122334455;
        } else if (hdr.ethernet.etherType == 0x88b8) {
            matches = 48w0x021122334455 != hdr.ethernet.dstAddr;
        } else if (hdr.ethernet.etherType == 0x88ba) {
            matches = hdr.ethernet.srcAddr < hdr.ethernet.dstAddr;
        } else if (hdr.ethernet.etherType == 0x88bb) {
            matches = hdr.ethernet.srcAddr <= hdr.ethernet.dstAddr;
        } else if (hdr.ethernet.etherType == 0x88bc) {
            matches = hdr.ethernet.srcAddr > hdr.ethernet.dstAddr;
        } else if (hdr.ethernet.etherType == 0x88bd) {
            matches = hdr.ethernet.srcAddr >= hdr.ethernet.dstAddr;
        }
        if (matches) {
            send_to_port(istd.input_port);
        } else {
            drop_packet();
        }
    }
}

PNA_NIC(ParserImpl(), ControlImpl(), DeparserImpl()) main;
