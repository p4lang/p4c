/*
 * SPDX-FileCopyrightText: 2019 Barefoot Networks, Inc.
 * Copyright 2019-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

struct H { };
struct M {
    bit<32> hash1;
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start { transition accept; }
}

action empty() { }

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {

    action drop() { mark_to_drop(smeta); }

    // For an action profile without an action selector, the 
    // `max_group_size`, `selector_size_semantics`, and `max_member_weight`
    // annotations are meaningless and should result in a warning.
    @name("ap") @max_group_size(200) 
    @selector_size_semantics("sum_of_members") @max_member_weight(4000)
    action_profile(32w128) my_action_profile;

    // For an action profile with an action selector using the `sum_of_members` 
    // size semantics, all of these annotations are meaningful and should result
    // in no warnings.
    @name("ap_ws") @max_group_size(200) 
    @selector_size_semantics("sum_of_members") @max_member_weight(4000)
    action_selector(HashAlgorithm.identity, 32w1024, 32w10)
        my_action_selector;

    table indirect {
        key = { }
        actions = { drop; NoAction; }
        const default_action = NoAction();
        implementation = my_action_profile;
    }

    table indirect_ws {
        key = { meta.hash1 : selector; }
        actions = { drop; NoAction; }
        const default_action = NoAction();
        implementation = my_action_selector;
    }

    apply {
        indirect.apply();
        indirect_ws.apply();
    }

};

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply { }
};

control DeparserI(packet_out pk, in H hdr) {
    apply { }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply { }
}

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(),
         ComputeChecksumI(), DeparserI()) main;
