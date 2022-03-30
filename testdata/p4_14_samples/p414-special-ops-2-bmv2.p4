/*
Copyright 2019 Cisco Systems, Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#define MAC_DA_DO_RESUBMIT    0x000000000001
#define MAC_DA_DO_RECIRCULATE 0x000000000002
#define MAC_DA_DO_CLONE_E2E   0x000000000003

#define MAX_RESUBMIT_COUNT    3
#define MAX_RECIRCULATE_COUNT 5
#define MAX_CLONE_E2E_COUNT   4

#define ETHERTYPE_VANILLA               0xf00f
#define ETHERTYPE_MAX_RESUBMIT          0xe50b
#define ETHERTYPE_MAX_RECIRCULATE       0xec14
#define ETHERTYPE_MAX_CLONE_E2E         0xce2e


header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type intrinsic_metadata_t {
    fields {
        ingress_global_timestamp : 48;
        egress_global_timestamp : 48;
        mcast_grp : 16;
        egress_rid : 16;
    }
}

header_type mymeta_t {
    fields {
        resubmit_count : 8;
        recirculate_count : 8;
        clone_e2e_count : 8;
        last_ing_instance_type : 8;
        f1 : 8;
    }
}

header_type temporaries_t {
    fields {
        temp1 : 48;
    }
}

header ethernet_t ethernet;
metadata intrinsic_metadata_t intrinsic_metadata;
metadata mymeta_t mymeta;
metadata temporaries_t temporaries;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

#define ENABLE_DEBUG_TABLE
#ifdef ENABLE_DEBUG_TABLE

        //standard_metadata.egress_instance: exact;
        //standard_metadata.parser_status: exact;
        //standard_metadata.parser_error: exact;

#define DEBUG_FIELD_LIST \
        standard_metadata.ingress_port: exact; \
        standard_metadata.packet_length: exact; \
        standard_metadata.egress_spec: exact; \
        standard_metadata.egress_port: exact; \
        standard_metadata.instance_type: exact; \
        intrinsic_metadata.ingress_global_timestamp: exact; \
        intrinsic_metadata.egress_global_timestamp: exact; \
        intrinsic_metadata.mcast_grp: exact; \
        intrinsic_metadata.egress_rid: exact; \
        mymeta.resubmit_count: exact; \
        mymeta.recirculate_count: exact; \
        mymeta.clone_e2e_count: exact; \
        mymeta.f1: exact; \
        mymeta.last_ing_instance_type : exact; \
        ethernet.dstAddr: exact; \
        ethernet.srcAddr: exact; \
        ethernet.etherType: exact;

table t_ing_debug_table1 {
    reads { DEBUG_FIELD_LIST }
    actions { _nop; }
    default_action: _nop;
}

table t_ing_debug_table2 {
    reads { DEBUG_FIELD_LIST }
    actions { _nop; }
    default_action: _nop;
}

table t_egr_debug_table1 {
    reads { DEBUG_FIELD_LIST }
    actions { _nop; }
    default_action: _nop;
}

table t_egr_debug_table2 {
    reads { DEBUG_FIELD_LIST }
    actions { _nop; }
    default_action: _nop;
}
#endif  // ENABLE_DEBUG_TABLE

field_list resubmit_FL {
    mymeta;
}

field_list recirculate_FL {
    mymeta;
}

field_list clone_e2e_FL {
    mymeta;
}

action do_resubmit() {
    subtract_from_field(ethernet.srcAddr, 17);
    add_to_field(mymeta.f1, 17);
    add_to_field(mymeta.resubmit_count, 1);
    resubmit(resubmit_FL);
}

table t_do_resubmit {
    reads { }
    actions { do_resubmit; }
    default_action: do_resubmit;
}

action mark_max_resubmit_packet () {
    modify_field(ethernet.etherType, ETHERTYPE_MAX_RESUBMIT);
}

table t_mark_max_resubmit_packet {
    reads { }
    actions { mark_max_resubmit_packet; }
    default_action: mark_max_resubmit_packet;
}

action set_port_to_mac_da_lsbs() {
    bit_and(standard_metadata.egress_spec, ethernet.dstAddr, 0xf);
}

table t_ing_mac_da {
    reads { }
    actions { set_port_to_mac_da_lsbs; }
    default_action: set_port_to_mac_da_lsbs;
}

action save_ing_instance_type () {
    modify_field(mymeta.last_ing_instance_type,
        standard_metadata.instance_type);
}

table t_save_ing_instance_type {
    reads { }
    actions { save_ing_instance_type; }
    default_action: save_ing_instance_type;
}

control ingress {
#ifdef ENABLE_DEBUG_TABLE
    apply(t_ing_debug_table1);
#endif  // ENABLE_DEBUG_TABLE
    if (ethernet.dstAddr == MAC_DA_DO_RESUBMIT) {
        if (mymeta.resubmit_count < MAX_RESUBMIT_COUNT) {
            apply(t_do_resubmit);
        } else {
            apply(t_mark_max_resubmit_packet);
        }
    } else {
        apply(t_ing_mac_da);
    }
    apply(t_save_ing_instance_type);
#ifdef ENABLE_DEBUG_TABLE
    apply(t_ing_debug_table2);
#endif  // ENABLE_DEBUG_TABLE
}

action put_debug_vals_in_eth_dstaddr () {
    // By copying values of selected metadata fields into the output
    // packet, we enable an automated STF test that checks the output
    // packet contents, to also check that the values of these
    // intermediate metadata field values are also correct.
    modify_field(ethernet.dstAddr, 0);
    shift_left(temporaries.temp1, mymeta.resubmit_count, 40);
    bit_or(ethernet.dstAddr, ethernet.dstAddr, temporaries.temp1);
    shift_left(temporaries.temp1, mymeta.recirculate_count, 32);
    bit_or(ethernet.dstAddr, ethernet.dstAddr, temporaries.temp1);
    shift_left(temporaries.temp1, mymeta.clone_e2e_count, 24);
    bit_or(ethernet.dstAddr, ethernet.dstAddr, temporaries.temp1);
    shift_left(temporaries.temp1, mymeta.f1, 16);
    bit_or(ethernet.dstAddr, ethernet.dstAddr, temporaries.temp1);
    shift_left(temporaries.temp1, mymeta.last_ing_instance_type, 8);
    bit_or(ethernet.dstAddr, ethernet.dstAddr, temporaries.temp1);
    // TBD: Commenting out the following lines, since the value that
    // they copy into the output packet may change soon, depending
    // upon the resolution of this issue:
    // https://github.com/p4lang/behavioral-model/issues/706
    //bit_and(temporaries.temp1, standard_metadata.instance_type, 0xff);
    //bit_or(ethernet.dstAddr, ethernet.dstAddr, temporaries.temp1);
}

action mark_egr_resubmit_packet () {
    put_debug_vals_in_eth_dstaddr();
}

table t_egr_mark_resubmit_packet {
    reads { }
    actions { mark_egr_resubmit_packet; }
    default_action: mark_egr_resubmit_packet;
}

action do_recirculate () {
    subtract_from_field(ethernet.srcAddr, 19);
    add_to_field(mymeta.f1, 19);
    add_to_field(mymeta.recirculate_count, 1);
    recirculate(recirculate_FL);
}

table t_do_recirculate {
    reads { }
    actions { do_recirculate; }
    default_action: do_recirculate;
}

action mark_max_recirculate_packet () {
    put_debug_vals_in_eth_dstaddr();
    modify_field(ethernet.etherType, ETHERTYPE_MAX_RECIRCULATE);
}

table t_mark_max_recirculate_packet {
    reads { }
    actions { mark_max_recirculate_packet; }
    default_action: mark_max_recirculate_packet;
}

action do_clone_e2e () {
    subtract_from_field(ethernet.srcAddr, 23);
    add_to_field(mymeta.f1, 23);
    add_to_field(mymeta.clone_e2e_count, 1);
    clone_egress_pkt_to_egress(1, clone_e2e_FL);
}

table t_do_clone_e2e {
    reads { }
    actions { do_clone_e2e; }
    default_action: do_clone_e2e;
}

action mark_max_clone_e2e_packet () {
    put_debug_vals_in_eth_dstaddr();
    modify_field(ethernet.etherType, ETHERTYPE_MAX_CLONE_E2E);
}

table t_mark_max_clone_e2e_packet {
    reads { }
    actions { mark_max_clone_e2e_packet; }
    default_action: mark_max_clone_e2e_packet;
}

action mark_vanilla_packet () {
    put_debug_vals_in_eth_dstaddr();
    modify_field(ethernet.etherType, ETHERTYPE_VANILLA);
}

table t_mark_vanilla_packet {
    reads { }
    actions { mark_vanilla_packet; }
    default_action: mark_vanilla_packet;
}

control egress {
#ifdef ENABLE_DEBUG_TABLE
    apply(t_egr_debug_table1);
#endif  // ENABLE_DEBUG_TABLE
    if (ethernet.dstAddr == MAC_DA_DO_RESUBMIT) {
        // Do nothing to the packet in egress, not even marking it
        // 'vanilla' as is done below.
        apply(t_egr_mark_resubmit_packet);
    } else if (ethernet.dstAddr == MAC_DA_DO_RECIRCULATE) {
        if (mymeta.recirculate_count < MAX_RECIRCULATE_COUNT) {
            apply(t_do_recirculate);
        } else {
            apply(t_mark_max_recirculate_packet);
        }
    } else if (ethernet.dstAddr == MAC_DA_DO_CLONE_E2E) {
        if (mymeta.clone_e2e_count < MAX_CLONE_E2E_COUNT) {
            apply(t_do_clone_e2e);
        } else {
            apply(t_mark_max_clone_e2e_packet);
        }
    } else {
        apply(t_mark_vanilla_packet);
    }
#ifdef ENABLE_DEBUG_TABLE
    apply(t_egr_debug_table2);
#endif  // ENABLE_DEBUG_TABLE
}
