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
#include "tofino/lpf_blackbox.p4"

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

action hop(ttl, egress_port) {
    add_to_field(ttl, -1);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
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

meter meter_0 {
    type : bytes;
    static : meter_tbl;
    result : ipv4.diffserv;
    instance_count : 500;
}

meter meter_1 {
    type : bytes;
    direct : meter_tbl_direct;
    result : ipv4.diffserv;
}


@pragma meter_pre_color_aware_per_flow_enable 1
meter meter_2 {
    type : bytes;
    static : meter_tbl_color_aware_indirect;
    result : ipv4.diffserv;
    pre_color : ipv4.diffserv;
    instance_count : 500;
}

meter meter_3 {
    type : bytes;
    direct : meter_tbl_color_aware_direct;
    result : ipv4.diffserv;
    pre_color : ipv4.diffserv;
}

blackbox lpf meter_lpf {
    filter_input : ipv4.srcAddr;
    instance_count : 500;
}


blackbox lpf meter_lpf_tcam {
   filter_input : ipv4.srcAddr; 
   static : match_tbl_tcam_lpf;
   instance_count : 500;
}

blackbox lpf meter_lpf_direct {
    filter_input : ipv4.srcAddr;
    direct : match_tbl_lpf_direct;
}    

blackbox lpf meter_lpf_tcam_direct {
    filter_input : ipv4.srcAddr;
    direct : match_tbl_tcam_lpf_direct;
}

action meter_action (egress_port, srcmac, dstmac, idx) {
    next_hop_ipv4(egress_port, srcmac, dstmac);
    execute_meter(meter_0, idx, ipv4.diffserv);
}

action meter_action_color_aware (egress_port, srcmac, dstmac, idx) {
    next_hop_ipv4(egress_port, srcmac, dstmac);
    execute_meter(meter_2, idx, ipv4.diffserv, ipv4.diffserv);
}

action count_color(color_idx) {
    count(colorCntr, color_idx);
}

action next_hop_ipv4_lpf(egress_port ,srcmac, dstmac, lpf_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    meter_lpf.execute(ipv4.srcAddr, lpf_idx);
}

action next_hop_ipv4_direct_lpf(egress_port ,srcmac, dstmac) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    meter_lpf_direct.execute(ipv4.srcAddr);
}

action next_hop_ipv4_lpf_tcam(egress_port ,srcmac, dstmac, lpf_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    meter_lpf_tcam.execute(ipv4.srcAddr, lpf_idx);
}

action next_hop_ipv4_lpf_direct_tcam(egress_port ,srcmac, dstmac) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    meter_lpf_tcam_direct.execute(ipv4.srcAddr);
}

counter colorCntr {
    type : packets;
    static : color_match;
    instance_count : 100;
}

@pragma command_line --no-dead-code-elimination
@pragma stage 1 
table meter_tbl {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        nop;
        meter_action;
    }
}

@pragma stage 0
table meter_tbl_direct {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }
    actions {
        nop;
        next_hop_ipv4;
    }
}

@pragma stage 2
table meter_tbl_color_aware_indirect {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }
    actions {
        nop;
        meter_action_color_aware;
    }
}


@pragma stage 3
table meter_tbl_color_aware_direct {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }
    actions {
        next_hop_ipv4;
    }
    default_action : nop();
}

@pragma stage 4
table match_tbl_lpf {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }
    actions {
        next_hop_ipv4_lpf;
    }
    default_action : nop();
}

@pragma stage 4
table match_tbl_tcam_lpf {
    reads {
        ipv4.dstAddr : ternary;
        ipv4.srcAddr : ternary;
    }
    actions {
        next_hop_ipv4_lpf_tcam;
    }
    default_action : nop();
}


@pragma stage 5
table match_tbl_lpf_direct {
    reads {
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
    }
    actions {
        next_hop_ipv4_direct_lpf;
    }
    default_action : nop();
}

@pragma stage 5
table match_tbl_tcam_lpf_direct {
    reads {
        ipv4.dstAddr : ternary;
        ipv4.srcAddr : ternary;
    }
    actions {
        next_hop_ipv4_lpf_direct_tcam;
    }
    default_action : nop();
}


@pragma stage 11
table color_match {
    reads {
        ethernet.dstAddr: exact;
        ipv4.diffserv: exact;
    }
    actions {
        count_color;
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
    apply(meter_tbl_direct);
    apply(meter_tbl);
    apply(meter_tbl_color_aware_indirect);
    apply(meter_tbl_color_aware_direct);
    apply(match_tbl_lpf);
    apply(match_tbl_tcam_lpf);
    apply(match_tbl_lpf_direct);
    apply(match_tbl_tcam_lpf_direct);
    apply(color_match);
}

control egress {

}

