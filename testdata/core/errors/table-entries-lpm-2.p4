/*
 * Copyright 2020 Cisco Systems, Inc.
 * SPDX-FileCopyrightText: 2020 Cisco Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<8>  e;
    bit<16> t;
    bit<8>  l;
    bit<8> r;
    bit<8>  v;
}

struct Header_t {
    hdr h;
}
struct Meta_t {}

parser p(packet_in b, out Header_t h, inout Meta_t m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Header_t h, inout Meta_t m) { apply {} }
control update(inout Header_t h, inout Meta_t m) { apply {} }
control egress(inout Header_t h, inout Meta_t m, inout standard_metadata_t sm) { apply {} }
control deparser(packet_out b, in Header_t h) { apply { b.emit(h.h); } }

control ingress(inout Header_t h, inout Meta_t m, inout standard_metadata_t standard_meta) {

    action a() { standard_meta.egress_spec = 0; }
    action a_with_control_params(bit<9> x) { standard_meta.egress_spec = x; }

    table t_lpm {

  	key = {
            h.h.l : lpm;
        }

	actions = {
            a;
            a_with_control_params;
        }

	default_action = a;

        const entries = {
            // Test detection of bad values and/or masks for keys with
            // match_kind lpm.

            // value has 1s outside of field bit width
            0x100 &&& 0xF0 : a_with_control_params(11);

            // mask has 1s outside of field bit width
            0x11 &&& 0x1F0 : a_with_control_params(12);

            // mask that is not a prefix mask
            0x11 &&& 0xE1 : a_with_control_params(13);

            // another mask that is not a prefix mask, and has 1 bits
            // outside of field bit width.
            0x11 &&& 0x181 : a_with_control_params(14);

            // exact match value with value having 1s outside of field
            // bit width
            0x100 : a_with_control_params(15);
        }
    }

    apply {
        t_lpm.apply();
    }
}


V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
