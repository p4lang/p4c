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

#include "tofino.p4"
#include "tofino/intrinsic_metadata.p4"
#include "tofino/constants.p4"

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

header_type ipv6_t {
    fields {
        version : 4;
        trafficClass : 8;
        flowLabel : 20;
        payloadLen : 16;
        nextHdr : 8;
        hopLimit : 8;
        srcAddr : 128;
        dstAddr : 128;
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
        0x86dd : parse_ipv6;        
        default: ingress;
    }
}

#define IP_PROTOCOLS_TCP 6
#define IP_PROTOCOLS_UDP 17

header ipv4_t ipv4;
header ipv6_t ipv6;

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
        default: ingress;
    }
}

parser parse_ipv6 {
    extract(ipv6);
    return select(latest.nextHdr) {
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
        default : ingress;
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

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    return ingress;
}

header udp_t udp;

parser parse_udp {
    extract(udp);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        drop: 1;
    }
}

metadata routing_metadata_t /*metadata*/ routing_metadata;

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

action egress_port(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
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

action drop_ipv4 () {
    drop();
}

action next_hop_ipv4(egress_port ,srcmac, dstmac) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
}

action next_hop_ipv6(egress_port ,srcmac, dstmac) {
    hop(ipv6.hopLimit, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
}

action ig_drop() {
//    modify_field(routing_metadata.drop, 1);
    add_to_field(ipv4.ttl, -1);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 1);
}

action mod_mac_addr(egress_port, srcmac, dstmac) {
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

action tcp_hdr_rm (egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    remove_header(tcp);
    modify_field(ipv4.protocol, 0);
//    add_to_field(ipv4.totalLen, -20);
//    modify_field(ipv4.totalLen, 66);
}

action modify_tcp_dst_port(dstPort) {
    modify_field(tcp.dstPort, dstPort);
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

action custom_action_4(egress_port, dstPort, srcPort)
{
    modify_field(tcp.dstPort, dstPort);
    modify_field(tcp.srcPort, srcPort);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action custom_action_5(dstPort, srcPort)
{
    modify_field(tcp.dstPort, dstPort);
    modify_field(tcp.srcPort, srcPort);
}

action switching_action_1(egress_port/*, vlan_id */)
{
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    //modify_field(vlan_tag.vlan_id, vlan_id);
}

action nhop_set(egress_port ,srcmac, dstmac) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
}

action nhop_set_1(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action hash_action(value) {
    modify_field(ipv4.ttl, value);
}

action hash_action2(value) {
    modify_field(vlan_tag.pri, value);
}

action_profile custom_action_3_profile {
    actions {
        nop;
        custom_action_3;
        egress_port;
    }
    size : 1024;
}

action_profile next_hop_ipv4_profile {
    actions {
        nop;
        next_hop_ipv4;
    }

    size : 2048;
}

action_profile next_hop_ipv4_1_profile {
    actions {
        nop;
        next_hop_ipv4;
    }

    size : 2048;
}

action_profile custom_action_1_profile {
    actions {
        nop;
        custom_action_1;
    }
    size : 2048;
}

action_profile custom_action_2_profile {
    actions {
        nop;
        custom_action_2;
    }
    size : 2048;
}

action_profile mod_mac_addr_profile {
    actions {
        nop;
        mod_mac_addr;
    }
    size : 1024;
}

action_profile modify_tcp_dst_port_1_profile {
    actions {
        nop;
        modify_tcp_dst_port_1;
    }
    size : 1024;
}

@pragma command_line --no-dead-code-elimination
@pragma stage 0
@pragma pack 5
@pragma ways 5
table exm_5ways_5Entries {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
        tcp.srcPort : exact;
    }

    action_profile : custom_action_3_profile;

    size : 25600;
}

@pragma stage 1
@pragma pack 5
@pragma ways 6

table exm_6ways_5Entries {
    reads {
        ethernet.dstAddr : exact;
        ipv4.dstAddr     : exact;
        tcp.dstPort      : exact;
    }

    action_profile : next_hop_ipv4_profile;

    size : 30720;
}

@pragma stage 2
@pragma pack 6
@pragma ways 4

table exm_4ways_6Entries {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }

    action_profile : custom_action_1_profile;

    size : 24576;
}

@pragma stage 3
@pragma pack 6
@pragma ways 5

table exm_5ways_6Entries {
    reads {
        ethernet.dstAddr : exact;
    }

    action_profile : custom_action_2_profile;

    size : 30720;
}

@pragma stage 4
@pragma pack 6
@pragma ways 6

table exm_6ways_6Entries {
    reads {
        ethernet.dstAddr : exact;
        tcp.srcPort      : exact;
    }

    action_profile : mod_mac_addr_profile;

    size : 36864;
}

@pragma stage 5
@pragma pack 7
@pragma ways 3

table exm_3ways_7Entries {
    reads {
        ipv4.dstAddr : exact;
    }

    action_profile : next_hop_ipv4_1_profile;

    size : 21504;
}

@pragma stage 6
@pragma pack 8
@pragma ways 4

table exm_4ways_8Entries {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }

    action_profile : modify_tcp_dst_port_1_profile;

    size : 32768;
}

@pragma stage 7

table exm_ipv4_routing {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }

    action_profile : next_hop_profile;

    size : 32768;
}

action_profile next_hop_profile {
    actions {
        nhop_set;
        nop;
    }
size : 4096;
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

@pragma stage 8
table ipv4_routing_select {
    reads {
            ipv4.dstAddr: exact;
    }
    action_profile : ecmp_action_profile;
}

@pragma stage 8
table ipv4_routing_select_iter {
    reads {
            ipv4.dstAddr: exact;
    }
    action_profile : ecmp_action_profile_iter;
}

@pragma stage 9
table stat_tbl_indirect_pkt_64bit {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
        act;
    }

    size : 2048;
}

@pragma stage 9
table stat_tbl_indirect_pkt_32bit {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
        act2;
    }

    size : 2048;
}

@pragma stage 9
table stat_tbl_indirect_pkt_byte_64bit {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
        act5;
    }

    size : 2048;
}

@pragma stage 10
table stat_tbl_direct_pkt_64bit {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
        act1;
    }

    size : 2048;
}

@pragma stage 10
table stat_tbl_direct_pkt_32bit {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
        act4;
    }

    size : 2048;
}

@pragma stage 11
table stat_tbl_direct_pkt_byte_64bit {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
        act1;
    }

    size : 2048;
}

@pragma stage 11
table stat_tbl_direct_pkt_byte_32bit {
    reads {
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    actions {
        act1;
    }

    size : 2048;
}

table stat_tcam_direct_pkt_64bit {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
        egr_act1;
    }

    size : 2048;
}

@pragma stage 11
table hash_action_exm {
    reads {
        ipv4.ttl : exact;
        ipv4 : valid;
    }
    actions {
        hash_action;
    }
    default_action : hash_action(33);
    size : 512;
}

@pragma stage 11
table hash_action2_exm {
    reads {
        vlan_tag.vlan_id : exact;
        vlan_tag.pri : exact;
    }
    actions {
        hash_action2;
    }
    default_action : hash_action2(1);
    size : 32768;
}

header_type l3_metadata_t {
    fields {
  	vrf : 24;
	fib_hit : 1;
	fib_nexthop : 16;
	fib_nexthop_type : 1;
    }  
}

metadata l3_metadata_t l3_metadata;

action fib_hit_nexthop(nexthop_index) {
    modify_field(l3_metadata.fib_hit, 1);
    modify_field(l3_metadata.fib_nexthop, nexthop_index);
    modify_field(l3_metadata.fib_nexthop_type, 0);
}

action fib_hit_ecmp(ecmp_index) {
    modify_field(l3_metadata.fib_hit, 1);
    modify_field(l3_metadata.fib_nexthop, ecmp_index);
    modify_field(l3_metadata.fib_nexthop_type, 1);
}

table duplicate_check_exm_immediate_action {
    reads {
        l3_metadata.vrf : exact;
        ipv4.dstAddr : exact;
    }
    actions {
        nop;
        fib_hit_nexthop;
        fib_hit_ecmp;
    }

    size : 2048;
}

action act(idx, egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    count(cntDum, idx);
}

action act1(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action egr_act1(tcp_sport) {
    modify_field(tcp.srcPort, tcp_sport);
}

action act4(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action act2(idx, egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    count(cntDum3, idx);
}

action act5(idx, egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    count(cntDum6, idx);
}

counter cntDum {
    type : packets;
    static : stat_tbl_indirect_pkt_64bit;
    instance_count : 256;
}

counter cntDum3 {
    type : packets;
    static : stat_tbl_indirect_pkt_32bit;
    instance_count : 256;
}

counter cntDum1 {
    type : packets;
    direct : stat_tbl_direct_pkt_64bit;
}

counter cntDum2 {
    type : packets;
    direct : stat_tbl_direct_pkt_32bit;
    min_width : 20;
}

counter cntDum4 {
    type : packets_and_bytes;
    direct : stat_tbl_direct_pkt_byte_64bit;
}

counter cntDum5 {
    type : packets_and_bytes;
    direct : stat_tbl_direct_pkt_byte_32bit;
    min_width : 20;
}

counter cntDum6 {
    type : packets_and_bytes;
    static : stat_tbl_indirect_pkt_byte_64bit;
    instance_count : 256;
}

counter egr_cntDum1 {
    type : packets;
    direct : stat_tcam_direct_pkt_64bit;
}

action_profile ecmp_action_profile {
    actions {
        nop;
        nhop_set_1;
    }
    size : 1024;
    // optional
    dynamic_action_selection : ecmp_selector;
}

action_profile ecmp_action_profile_iter {
    actions {
        nop;
        nhop_set_1;
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

action set_ucast_dest(dest) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, dest);
}

@pragma stage 11
table exm_txn_test {
    reads {
        ig_intr_md.ingress_port : exact;
    }
    actions {
        set_ucast_dest;
    }
    size : 256;
}

/* Main control flow */
control ingress {
    /* A general principle : Always keep the exact match tables ahead of the 
     * ternary tables in the same stage, except first stage. Logic relating to next-table
     * will cause the Tofino model not to launch a lookup on th exact match
     * tables if the order is reversed.
     */
    apply(exm_5ways_5Entries);
    apply(exm_6ways_5Entries);
    apply(exm_4ways_6Entries);
    apply(exm_5ways_6Entries);
    apply(exm_6ways_6Entries);
    apply(exm_3ways_7Entries);
    apply(exm_4ways_8Entries);
    apply(exm_ipv4_routing); /* Explicitly managed action data entries */
    apply(ipv4_routing_select);
    apply(ipv4_routing_select_iter);
    apply(stat_tbl_indirect_pkt_64bit);
    apply(stat_tbl_indirect_pkt_32bit);
    apply(stat_tbl_indirect_pkt_byte_64bit);
    apply(stat_tbl_direct_pkt_64bit);
    apply(stat_tbl_direct_pkt_32bit);
    apply(stat_tbl_direct_pkt_byte_64bit);
    apply(stat_tbl_direct_pkt_byte_32bit);
    /* Hash-action tables are always executed. Using gateway condition
       of tcp-srcport to prevent all tests from hitting this condition
    */
    if (tcp.srcPort == 9000) {
        apply(hash_action_exm);
        apply(hash_action2_exm);
    }
    apply(exm_txn_test);
}

action eg_drop() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 0);
    modify_field(intrinsic_metadata.egress_port, 0);
}

action permit() {
}

table egress_acl {
    reads {
        routing_metadata.drop: ternary;
    }
    actions {
        permit;
        eg_drop;
    }
}

action action1() {
    add_to_field(ipv4.ttl, -1);
}

table exm_txn_test1 {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        action1;
    }
    size : 4096;
}

control egress {
      apply(stat_tcam_direct_pkt_64bit);
      apply(duplicate_check_exm_immediate_action);
      apply(exm_txn_test1);
//    apply(egress_acl);
}

