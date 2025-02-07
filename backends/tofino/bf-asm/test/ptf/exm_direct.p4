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

#include "tofino_exm.p4"
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
    actions {
        nop;
        custom_action_3;
    }

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
    actions {
        nop;
        next_hop_ipv4;
    }
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
    actions {
        nop;
        custom_action_1;
    }
    size : 24576;
}

@pragma stage 3
@pragma pack 6
@pragma ways 5

table exm_5ways_6Entries {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        custom_action_2;
    }
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
    actions {
        nop;
        mod_mac_addr;
    }
    size : 36864;
}

@pragma stage 5
@pragma pack 7
@pragma ways 3

table exm_3ways_7Entries {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        nop;
        next_hop_ipv4;
    }
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
    actions {
        nop;
        modify_tcp_dst_port_1;
    }
    size : 32768;
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
    output_width : 72;
}

@pragma stage 7
@pragma selector_max_group_size 119040
//@pragma selector_max_group_size 200
table tcp_port_select {
    reads {
            ipv4.dstAddr: lpm;
    }
    action_profile : tcp_port_action_profile;
    size : 512;
}

action_profile tcp_port_action_profile {
    actions {
        tcp_port_modify;
        nop;
    }
    size : 131072;
//    size : 1024;
    dynamic_action_selection : ecmp_selector;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash; // take a field_list_calculation only
    // optional
    selection_mode : resilient; // “resilient” or “non-resilient”
}

@pragma stage 8
//@pragma selector_max_group_size 119040
@pragma selector_max_group_size 100
table tcp_port_select_exm {
    reads {
            ipv4.dstAddr: exact;
    }
    action_profile : tcp_port_action_profile_exm;
    size : 512;
}

action_profile tcp_port_action_profile_exm {
    actions {
        tcp_port_modify;
        nop;
    }
//    size : 131072;
    size : 2048;
    dynamic_action_selection : fair_selector;
}

action_selector fair_selector {
    selection_key : ecmp_hash; // take a field_list_calculation only
    // optional
    selection_mode : non_resilient; // “resilient” or “non-resilient”
}

action tcp_port_modify(sPort, port) {
    modify_field(tcp.srcPort, sPort);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

@pragma immediate 1
@pragma stage 9 
@pragma include_idletime 1
@pragma idletime_precision 2
table idle_tcam_2 {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 512;
    support_timeout: true;
}

@pragma immediate 1
@pragma stage 9
@pragma include_idletime 1
@pragma idletime_precision 6 
table idle_tcam_6 {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 512;
    support_timeout: true;
}

@pragma immediate 1
@pragma stage 9
@pragma include_idletime 1
@pragma idletime_precision 3 
@pragma idletime_per_flow_idletime 0 
table idle_tcam_3d {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 512;
    support_timeout: true;
}

@pragma immediate 1
@pragma stage 9
@pragma include_idletime 1
@pragma idletime_precision 6 
@pragma idletime_per_flow_idletime 0 
table idle_tcam_6d {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 512;
    support_timeout: true;
}

@pragma stage 9
@pragma pack 1
@pragma ways 4
table exm_movereg {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        nop;
        next_hop_ipv4;
    }
    size : 4096;
}

counter exm_cnt {
    type : packets;
    direct : exm_movereg;
}

@pragma immediate 1
@pragma stage 10 
@pragma include_idletime 1
@pragma idletime_precision 1
table idle_tcam_1 {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 512;
    support_timeout: true;
}

@pragma immediate 1
@pragma stage 10 
@pragma include_idletime 1
@pragma idletime_precision 2
table idle_2 {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 1024;
    support_timeout: true;
}

@pragma immediate 1
@pragma stage 10 
@pragma include_idletime 1
@pragma idletime_precision 3
table idle_3 {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 1024;
    support_timeout: true;
}

@pragma immediate 1
@pragma stage 10 
@pragma include_idletime 1
@pragma idletime_precision 6
table idle_6 {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 1024;
    support_timeout: true;
}

@pragma immediate 1
@pragma stage 11
@pragma include_idletime 1
@pragma idletime_precision 3 
table idle_tcam_3 {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
      nop;
      hop_ipv4;
    }
    size : 12288;
    support_timeout: true;
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
    //Stage 7
    apply(tcp_port_select);
    // Stage 8
    apply(tcp_port_select_exm);
    // Stage 9
    apply(idle_tcam_2);
    apply(idle_tcam_6);
    apply(idle_tcam_3d);
    apply(idle_tcam_6d);
    apply(exm_movereg);
    //Stage 10
    apply(idle_tcam_1);
    apply(idle_2);
    apply(idle_3);
    apply(idle_6);
    //Stage 11
    apply(idle_tcam_3);
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

control egress {
//    apply(egress_acl);
}
