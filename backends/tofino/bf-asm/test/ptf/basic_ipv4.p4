/*
Copyright 2013-present Barefoot Networks, Inc. 

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

#include <tofino/intrinsic_metadata.p4>
#include <tofino/constants.p4>

/* Sample P4 program */
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pri     : 3;
        cfi     : 1;
        vlan_id : 12;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        hdr_length : 16;
        checksum : 16;
    }
}

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x8100 : parse_vlan_tag;
        0x800 : parse_ipv4;
        default: ingress;
    }
}

#define IP_PROTOCOLS_TCP 6
#define IP_PROTOCOLS_UDP 17

header ipv4_t ipv4;

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
        default: ingress;
    }
}

header vlan_tag_t vlan_tag;

parser parse_vlan_tag {
    extract(vlan_tag);
    return select(latest.etherType) {
        0x800 : parse_ipv4;
        default : ingress;
    }
}

header_type md_t {
    fields {
        sport:16;
        dport:16;
    }
}

metadata md_t md;

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    set_metadata(md.sport, latest.srcPort);
    set_metadata(md.dport, latest.dstPort);
    return ingress;
}

header udp_t udp;

parser parse_udp {
    extract(udp);
    set_metadata(md.sport, latest.srcPort);
    set_metadata(md.dport, latest.dstPort);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        drop: 1;
        learn_meta_1:20;
        learn_meta_2:24;
        learn_meta_3:25;
        learn_meta_4:10;
    }
}

metadata routing_metadata_t /*metadata*/ routing_metadata;

header_type range_metadata_t {
    fields {
        src_range_index : 16;
        dest_range_index : 16;
    }
}

metadata range_metadata_t range_mdata;

field_list ipv4_field_list {
    ipv4.version;
    ipv4.ihl;
    ipv4.diffserv;
    ipv4.totalLen;
    ipv4.identification;
    ipv4.flags;
    ipv4.fragOffset;
    ipv4.ttl;
    ipv4.protocol;
    ipv4.srcAddr;
    ipv4.dstAddr;
}

field_list_calculation ipv4_chksum_calc {
    input {
        ipv4_field_list;
    }
    algorithm : csum16;
    output_width: 16;
}

calculated_field ipv4.hdrChecksum {
    update ipv4_chksum_calc;
}

action nop() {
}

field_list learn_1 {
    ipv4.ihl;
    ipv4.protocol;
    ipv4.srcAddr;
    ethernet.srcAddr;
    ethernet.dstAddr;
    ipv4.fragOffset;
    ipv4.identification;
    routing_metadata.learn_meta_1;
    routing_metadata.learn_meta_4;
}

action learn_1 (learn_meta_1, learn_meta_4) {
    generate_digest(FLOW_LRN_DIGEST_RCVR, learn_1);
    modify_field(routing_metadata.learn_meta_1, learn_meta_1);
    modify_field(routing_metadata.learn_meta_4, learn_meta_4);
}

field_list learn_2 {
    ipv4.ihl;
    ipv4.identification;
    ipv4.protocol;
    ipv4.srcAddr;
    ethernet.srcAddr;
    ethernet.dstAddr;
    ipv4.fragOffset;
    routing_metadata.learn_meta_2;
    routing_metadata.learn_meta_3;
}

action learn_2 (learn_meta_2, learn_meta_3) {
    generate_digest(FLOW_LRN_DIGEST_RCVR, learn_2);
    modify_field(routing_metadata.learn_meta_2, learn_meta_2);
    modify_field(routing_metadata.learn_meta_3, learn_meta_3);
}

action hop(ttl, egress_port) {
    add_to_field(ttl, -1);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action hop_ipv4(egress_port /*,srcmac, dstmac*/) {
    hop(ipv4.ttl, egress_port);
//    modify_field(ethernet.srcAddr, srcmac);
//    modify_field(ethernet.dstAddr, dstmac);
}

action set_srange_mdata(index) {
    modify_field(range_mdata.src_range_index, index);
}

action set_drange_mdata(index) {
    modify_field(range_mdata.dest_range_index, index);
}

action drop_ipv4 () {
    drop();
}

action next_hop_ipv4(egress_port, srcmac, dstmac) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
}

action mod_mac_adr(egress_port, srcmac, dstmac) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
}

action udp_hdr_add (egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    add_header(udp);
    modify_field(ipv4.protocol, IP_PROTOCOLS_UDP);
    add_to_field(ipv4.totalLen, 8);
}

action udp_set_dest(port) {
    modify_field(udp.dstPort, port);
}
action udp_set_src(port) {
    modify_field(udp.srcPort, port);
}
action tcp_set_src_dest(sport, dport) {
    modify_field(tcp.srcPort, sport);
    modify_field(tcp.dstPort, dport);
}

action tcp_hdr_rm (egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    remove_header(tcp);
    modify_field(ipv4.protocol, 0);
//    add_to_field(ipv4.totalLen, -20);
//    modify_field(ipv4.totalLen, 66);
}

action modify_tcp_dst_port_1(dstPort, egress_port) {
    modify_field(tcp.dstPort, dstPort);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action custom_action_1(egress_port, ipAddr, dstAddr, tcpPort)
{
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    modify_field(ipv4.srcAddr, ipAddr);
    modify_field(ethernet.dstAddr, dstAddr);
    modify_field(tcp.dstPort, tcpPort);
}

action custom_action_2(egress_port, ipAddr, tcpPort)
{
    modify_field(ipv4.srcAddr, ipAddr);
    modify_field(tcp.dstPort, tcpPort);
    hop(ipv4.ttl, egress_port);
}

action custom_action_3(egress_port, dstAddr, dstIp)
{
    modify_field(ipv4.dstAddr, dstIp);
    modify_field(ethernet.dstAddr, dstAddr);
    hop(ipv4.ttl, egress_port);
}

action modify_l2 (egress_port, srcAddr, dstAddr, tcp_sport, tcp_dport) {
    // Trying for >128 bit action data
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcAddr);
    modify_field(ethernet.dstAddr, dstAddr);
    modify_field(tcp.dstPort, tcp_dport);
    modify_field(tcp.srcPort, tcp_sport);
}

@pragma command_line --no-dead-code-elimination
@pragma immediate 1
@pragma stage 0
table ig_udp {
    reads {
        ethernet    : valid;
        ipv4        : valid;
        udp         : valid;
        udp.dstPort : ternary;
    }
    actions {
      nop;
      udp_set_dest;
    }
}
@pragma immediate 1
@pragma stage 0
table eg_udp {
    reads {
        ethernet    : valid;
        ipv4        : valid;
        udp         : valid;
        udp.srcPort : exact;
    }
    actions {
      nop;
      udp_set_src;
    }
}
table ig_udp_ternary_valid {
    reads {
        ethernet   : valid;
        ipv4.valid : exact;
        udp.valid  : ternary;
        md.dport   : ternary;
    }
    actions {
      nop;
      udp_set_dest;
      tcp_set_src_dest;
    }
}

@pragma immediate 1
@pragma stage 0
table ipv4_routing {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      hop_ipv4;
      drop_ipv4;
      learn_1;
      learn_2;
    }
    size : 512;
}

@pragma immediate 1
@pragma stage 0
@pragma ways 3
@pragma pack 5
table ipv4_routing_exm_ways_3_pack_5 {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }
    actions {
        nop;
        modify_tcp_dst_port_1;
    }
}


@pragma immediate 1
@pragma stage 1
@pragma ways 3
@pragma pack 3
table ipv4_routing_exm_ways_3_pack_3 {
    reads {
        ipv4.dstAddr : exact;
        ethernet.dstAddr : exact;
    }
    actions {
      nop;
      custom_action_1;
    }
}

@pragma immediate 1
@pragma stage 1
@pragma ways 4
@pragma pack 3
table ipv4_routing_exm_ways_4_pack_3_stage_1 {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }
    actions {
      nop;
      next_hop_ipv4;
    }
}

@pragma immediate 1
@pragma stage 1
table ipv4_routing_stage_1 {
    reads {
        ipv4.dstAddr : lpm;
        ipv4.srcAddr : exact;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 1024;
}

@pragma stage 2
table tcam_tbl_stage_2 {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      mod_mac_adr;
    }
}

@pragma stage 2
@pragma ways 4
@pragma pack 7
table ipv4_routing_exm_ways_4_pack_7_stage_2 {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
        tcp.dstPort  : exact;
        tcp.srcPort  : exact;
    }
    actions {
      nop;
      custom_action_2;
    }
}

@pragma immediate 1
@pragma stage 3
@pragma ways 5
@pragma pack 3
table ipv4_routing_exm_ways_5_pack_3_stage_3 {
    reads {
        ipv4.dstAddr     : exact;
        ethernet.srcAddr : exact;
    }
    actions {
      nop;
      next_hop_ipv4;
    }
}

@pragma immediate 1
@pragma stage 3
table udp_add_tbl_stage_3 {
    reads {
        ethernet.srcAddr : ternary;
    }
    actions {
        nop;
        udp_hdr_add;
    }
}

@pragma immediate 1
@pragma stage 4
@pragma ways 6
@pragma pack 3
table ipv4_routing_exm_ways_6_pack_3_stage_4 {
    reads {
        ipv4.dstAddr : exact;
        ethernet.dstAddr : exact;
    }
    actions {
      nop;
      next_hop_ipv4;
    }
}


@pragma immediate 1
@pragma stage 4
table tcp_rm_tbl_stage_4 {
    reads {
        ethernet.srcAddr : ternary;
    }
    actions {
        nop;
        tcp_hdr_rm;
    }
}

@pragma immediate 1
@pragma stage 5
@pragma ways 3
@pragma pack 4
table ipv4_routing_exm_ways_3_pack_4_stage_5 {
    reads {
        ipv4.srcAddr : exact;
        ethernet.dstAddr : exact;
        tcp.dstPort : exact;
    }
    actions {
      nop;
      modify_tcp_dst_port_1;
    }
}

@pragma stage 6
@pragma ways 4
@pragma pack 4
table ipv4_routing_exm_ways_4_pack_4_stage_6 {
    reads {
        ethernet.srcAddr : exact;
        ethernet.dstAddr : exact;
        tcp.srcPort : exact;
    }
    actions {
      nop;
      next_hop_ipv4;
    }
}

@pragma immediate 1
@pragma stage 7
@pragma ways 5
@pragma pack 4
table ipv4_routing_exm_ways_5_pack_4_stage_7 {
    reads {
        ipv4.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
      nop;
      custom_action_3;
    }
}

@pragma immediate 1
@pragma stage 8
table tcam_adt_deep_stage_8 {
    reads {
        ethernet.srcAddr : ternary;
        ethernet.dstAddr : ternary;
        ipv4.srcAddr     : ternary;
        ipv4.dstAddr     : ternary;
    }
    actions {
        nop;
        modify_l2;
    }
    size : 3072;
}

@pragma stage 8
@pragma ways 4
@pragma pack 5
table ipv4_routing_exm_ways_4_pack_5_stage_8 {
    reads {
        ipv4.srcAddr     : exact;
        ethernet.dstAddr : exact;
        tcp.srcPort      : exact;
        tcp.dstPort      : exact;
    }
    actions {
        nop;
        custom_action_2;
    }
}

@pragma stage 9
table ipv4_routing_select {
    reads {
            ipv4.dstAddr: lpm;
    }
    action_profile : ecmp_action_profile;
    size : 512;
}

field_list ecmp_hash_fields {
    ipv4.srcAddr;
    ipv4.dstAddr;
    ipv4.identification;
    ipv4.protocol;
}

field_list_calculation ecmp_hash {
    input {
        ecmp_hash_fields;
    }
#if defined(BMV2TOFINO)
    algorithm : xxh64;
#else
    algorithm : random;
#endif
    output_width : 64;
}

action_profile ecmp_action_profile {
    actions {
        nhop_set;
        nop;
    }
    size : 1024;
    // optional
    dynamic_action_selection : ecmp_selector;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash; // take a field_list_calculation only
    // optional
    selection_mode : resilient; // “resilient” or “non-resilient”
}

action nhop_set(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

@pragma stage 10
table tcam_indirect_action {
    reads {
        ethernet.srcAddr : ternary;
        ethernet.dstAddr : ternary;
        ethernet.etherType: exact;
        ipv4.srcAddr     : ternary;
        ipv4.dstAddr     : ternary;
        ipv4.protocol    : exact;
        ipv4.version     : exact;
    }
    action_profile : indirect_action_profile;
    size : 2048;
}

action modify_ip_id(port, id, srcAddr, dstAddr) {
    modify_field(ipv4.identification, id);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
    modify_field(ethernet.srcAddr, srcAddr);
    modify_field(ethernet.dstAddr, dstAddr);
}

action_profile indirect_action_profile {
    actions {
        nop;
        modify_ip_id;
    }
    size : 1500;
}

@pragma stage 11
@pragma ways 6
@pragma pack 4
table ipv4_routing_exm_ways_6_pack_4_stage_11 {
    reads {
        ipv4.dstAddr : exact;
        tcp.dstPort  : exact;
    }
    actions {
      nop;
      next_hop_ipv4;
    }
}

@pragma stage 5
table tcam_range {
    reads {
        ipv4.dstAddr : ternary;
        tcp.dstPort : range;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 1024;
}

@pragma stage 5
table tcam_range_ternary_valid {
    reads {
        tcp.valid : ternary;
        ipv4.dstAddr : ternary;
        md.dport : range;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 1024;
}

@pragma stage 6 
@pragma entries_with_ranges 1
table tcam_range_2_fields {
    reads {
        ipv4.dstAddr : ternary;
        tcp.dstPort : range;
        tcp.srcPort : range;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 1024;
}

@pragma stage 7 
table src_non_overlap_range_table{
    reads {
        tcp.srcPort : range;
    }
    actions {
        nop;
        set_srange_mdata;
    }
    size : 1024;
}

@pragma stage 7
table dest_non_overlap_range_table{
    reads {
        tcp.dstPort : range;
    }
    actions {
        nop;
        set_drange_mdata;
    }
    size : 1024;
}

@pragma stage 11
@pragma entries_with_ranges 1
table match_range_table{
    reads {
        range_mdata.src_range_index : ternary;
        range_mdata.dest_range_index : ternary;
        tcp.srcPort : range;
        tcp.dstPort : range;
        ipv4.dstAddr : ternary;
        ipv4.srcAddr : ternary;
    }
    actions {
        nop;
        hop_ipv4;
    }
    size : 1024;
}

action set_mgid() {
    modify_field(ig_intr_md_for_tm.mcast_grp_a, 0xAAAA);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0x5555);
}
action clr_mgid() {
    bit_andca(ig_intr_md_for_tm.mcast_grp_a, ig_intr_md_for_tm.mcast_grp_a, ig_intr_md_for_tm.mcast_grp_a);
    bit_andca(ig_intr_md_for_tm.mcast_grp_b, ig_intr_md_for_tm.mcast_grp_b, ig_intr_md_for_tm.mcast_grp_b);
}
@pragma stage 10
table no_key_1 {
    actions {
        set_mgid;
    }
}
@pragma stage 11
table no_key_2 {
    actions {
        clr_mgid;
    }
}

/* Main control flow */
control ingress {
    /* A general principle : Always keep the exact match tables ahead of the 
     * ternary tables in the same stage, except first stage. Logic relating to next-table
     * will cause the Tofino model not to launch a lookup on th exact match
     * tables if the order is reversed.
     */
    // stage 0
    apply(ig_udp);
    apply(ig_udp_ternary_valid);
    apply(ipv4_routing);
    apply(ipv4_routing_exm_ways_3_pack_5);
    // stage 1
    apply(ipv4_routing_exm_ways_3_pack_3);
    apply(ipv4_routing_exm_ways_4_pack_3_stage_1);
    apply(ipv4_routing_stage_1);
    // stage 2
    apply(tcam_tbl_stage_2);
    apply(ipv4_routing_exm_ways_4_pack_7_stage_2);
    // stage 3
    apply(ipv4_routing_exm_ways_5_pack_3_stage_3);
    /* Gateway not yet supported */
//    if (ipv4.protocol == 0) {
    apply(udp_add_tbl_stage_3);
//    }

    // stage 4
    apply(ipv4_routing_exm_ways_6_pack_3_stage_4);
//    if (valid(tcp)) {
    apply(tcp_rm_tbl_stage_4);
//    }

    // stage 5
    apply(ipv4_routing_exm_ways_3_pack_4_stage_5);
    apply(tcam_range);
    apply(tcam_range_ternary_valid);
    // stage 6
    apply(ipv4_routing_exm_ways_4_pack_4_stage_6);
    apply(tcam_range_2_fields);
    // stage 7
    apply(ipv4_routing_exm_ways_5_pack_4_stage_7);
    apply(src_non_overlap_range_table);
    apply(dest_non_overlap_range_table);
    // stage 8
    apply(tcam_adt_deep_stage_8);
    apply(ipv4_routing_exm_ways_4_pack_5_stage_8);
    // Stage 9 
    apply(ipv4_routing_select);
    // Stage 10
    apply(tcam_indirect_action);
    apply(no_key_1);
    // Stage 11
    apply(match_range_table);
    apply(no_key_2);
}

control egress {
    apply(eg_udp);
    // Commenting out since modify of egress port is not possible in egress
//    apply(ipv4_routing_exm_ways_6_pack_4_stage_11);
}

