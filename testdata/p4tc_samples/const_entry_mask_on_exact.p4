/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <tc/pna.p4>

header hdr {
    bit<8> e;
}

struct Header_t {
    hdr h;
}
struct Meta_t {}

parser MainParserImpl(packet_in b, out Header_t h, inout Meta_t m, in pna_main_parser_input_metadata_t istd) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control MainControlImpl(inout Header_t h, inout Meta_t m,
                        in    pna_main_input_metadata_t istd,
                        inout pna_main_output_metadata_t ostd) {

    action a_with_control_params(bit<8> x) { h.h.e = x; }

    // An exact key is compared in full, so a masked keyset denotes a set of
    // values and each of them becomes an entry of its own.
    table t_exact {
        key = {
            h.h.e : exact;
        }

        actions = {
            a_with_control_params;
        }

        default_action = a_with_control_params(0);

        const entries = {
            0x0B &&& 0xF0 : a_with_control_params(1);
        }
    }

    apply {
        t_exact.apply();
    }
}

/*********************  D E P A R S E R  ************************/

control MainDeparserImpl(
    packet_out b, inout Header_t h, in Meta_t m, in pna_main_output_metadata_t ostd)
{
    apply {}
}

/************ F I N A L   P A C K A G E  ******************************/

PNA_NIC(
    MainParserImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
