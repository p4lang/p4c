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
        if (istd.parser_error == error.NoError) {
            send_to_port(istd.input_port);
        } else {
            drop_packet();
        }
    }
}

PNA_NIC(ParserImpl(), ControlImpl(), DeparserImpl()) main;
