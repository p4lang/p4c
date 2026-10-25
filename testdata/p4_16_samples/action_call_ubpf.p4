/*
 * Copyright 2019 Orange
 * SPDX-FileCopyrightText: 2019 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <ubpf_model.p4>

struct Headers_t {

}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action RejectConditional(bit<1> condition) {
        if (condition == 1) {
            bit<1> tmp = 0;  // does not matter
        } else {
            bit<1> tmp = 0;  // does not matter
        }
    }

    action act_return() {
        mark_to_pass();
        return;
        bit<48> tmp = ubpf_time_get_ns();  // this should not be invoked.
    }

    action act_exit() {
        mark_to_pass();
        exit;
        bit<48> tmp = ubpf_time_get_ns();  // this should not be invoked.
    }

    table tbl_a {
        actions = { RejectConditional();
                    act_return();
                    act_exit();}
        default_action = RejectConditional(0);
    }

    apply {
        tbl_a.apply();

        exit;
        return;
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;