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

#include "tofino/intrinsic_metadata.p4"
#include "tofino/constants.p4"
#include "p4features.h"

#if defined(STATEFUL_DIRECT) || defined(STATEFUL_INDIRECT)
#include "tofino/stateful_alu_blackbox.p4"
#endif

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

#if defined(STATS_DIRECT) || defined(STATS_INDIRECT)
counter cntr {
    type : packets;
#ifdef STATS_DIRECT
    direct : match_tbl;
#elif defined(STATS_INDIRECT)
//    static : match_tbl;
    instance_count : STATS_COUNT;
#endif
}
#endif

#if defined(METER_DIRECT) || defined(METER_INDIRECT)
meter mtr {
    type : bytes;
#ifdef METER_DIRECT
    direct : match_tbl;
//    result : ig_intr_md_for_tm.packet_color;
    result : ipv4.diffserv;
#elif defined(METER_INDIRECT)
//    static : match_tbl;
//    result : ig_intr_md_for_tm.packet_color;
//    result : ipv4.diffserv;
    instance_count : METER_COUNT;
#endif
}
#endif

#if defined(STATEFUL_DIRECT)
register r {
    width  : 32;
    direct : match_tbl;
}
blackbox stateful_alu r_alu {
    reg: r;
    initial_register_lo_value: 1;
    update_lo_1_value: register_lo + 1;
}
#endif

#if defined(STATEFUL_INDIRECT)
register r {
    width  : 32;
    instance_count: STATEFUL_COUNT;
}
blackbox stateful_alu r_alu1 {
    reg: r;
    initial_register_lo_value: 1;
    update_lo_1_value: register_lo + 1;
}
blackbox stateful_alu r_alu2 {
    reg: r;
    initial_register_lo_value: 1;
    update_lo_1_value: register_lo + 100;
}
blackbox stateful_alu r_alu3 {
    reg: r;
    initial_register_lo_value: 1;
    update_lo_1_value: register_lo + 1234;
}
blackbox stateful_alu r_alu4 {
    reg: r;
    initial_register_lo_value: 1;
    update_lo_1_value: register_lo + 333;
}
#endif


action tcp_sport_modify (sPort, port) {
    modify_field(tcp.srcPort, sPort);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
#ifdef STATEFUL_DIRECT
    r_alu.execute_stateful_alu();
#endif
}

action tcp_dport_modify (dPort, port
#ifdef STATEFUL_INDIRECT
                         , stful_idx
#endif
                        ) {
    modify_field(tcp.dstPort, dPort);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
#ifdef STATEFUL_INDIRECT
    r_alu1.execute_stateful_alu(stful_idx);
#endif
#ifdef STATEFUL_DIRECT
    r_alu.execute_stateful_alu();
#endif
}

action ipsa_modify (ipsa, port
#ifdef STATS_INDIRECT
        ,stat_idx
#endif
#ifdef METER_INDIRECT
        , meter_idx
#endif
#ifdef STATEFUL_INDIRECT
        , stful_idx
#endif
        ) {
    modify_field(ipv4.srcAddr, ipsa);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
#ifdef STATS_INDIRECT
    count(cntr, stat_idx);
#endif
#ifdef METER_INDIRECT
//    execute_meter(mtr, meter_idx, ig_intr_md_for_tm.packet_color);
    execute_meter(mtr, meter_idx, ipv4.diffserv);
#endif
#ifdef STATEFUL_INDIRECT
    r_alu1.execute_stateful_alu(stful_idx);
#endif
#ifdef STATEFUL_DIRECT
    r_alu.execute_stateful_alu();
#endif
}

action ipda_modify (ipda, port
#ifdef STATEFUL_INDIRECT
                    , stful_idx
#endif
                   ) {
    modify_field(ipv4.dstAddr, ipda);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
#ifdef STATEFUL_INDIRECT
    r_alu2.execute_stateful_alu(stful_idx);
#endif
#ifdef STATEFUL_DIRECT
    r_alu.execute_stateful_alu();
#endif
}

action ipds_modify(ds, port
#ifdef STATEFUL_INDIRECT
                   , stful_idx
#endif
                  ) {
    modify_field(ipv4.diffserv, ds);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
#ifdef STATEFUL_INDIRECT
    r_alu3.execute_stateful_alu(stful_idx);
#endif
#ifdef STATEFUL_DIRECT
    r_alu.execute_stateful_alu();
#endif
}

action ipttl_modify(ttl, port
#ifdef STATEFUL_INDIRECT
                    , stful_idx
#endif
                   ) {
    modify_field(ipv4.ttl, ttl);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
#ifdef STATEFUL_INDIRECT
    r_alu4.execute_stateful_alu(stful_idx);
#endif
#ifdef STATEFUL_DIRECT
    r_alu.execute_stateful_alu();
#endif
}

#ifdef SELECTOR_INDIRECT
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
    algorithm : random;
    output_width : 72;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash; // take a field_list_calculation only
    // optional
    selection_mode : resilient; // “resilient” or “non-resilient”
}
#endif

#ifdef ACTION_INDIRECT
#ifdef STATS_INDIRECT
@pragma bind_indirect_res_to_match cntr 
#endif
#ifdef METER_INDIRECT
@pragma bind_indirect_res_to_match mtr 
#endif
#ifdef STATEFUL_INDIRECT
@pragma bind_indirect_res_to_match r 
#endif
action_profile selector_profile {
    actions {
        tcp_sport_modify;
        tcp_dport_modify;
        ipsa_modify;
        ipda_modify;
        ipds_modify;
        ipttl_modify;
    }
    size : ACTION_COUNT;
#ifdef SELECTOR_INDIRECT
    dynamic_action_selection : ecmp_selector;
#endif
}
#endif

#ifdef MATCH_EXM
#if MATCH_COUNT > 40000
@pragma stage 0 8192
@pragma stage 1 6144
@pragma stage 2 8192
@pragma stage 3 6144
@pragma stage 4 8192
@pragma stage 5 6144
@pragma stage 6 8192
@pragma stage 7 6144
@pragma stage 8 8192
@pragma stage 9 6144
@pragma stage 10 8192
@pragma stage 11
#endif
#endif
#ifdef MATCH_ATCAM
@pragma atcam_partition_index vlan_tag.vlan_id
#endif
#ifdef MATCH_ALPM
@pragma alpm 1
#endif
#ifdef SELECTOR_INDIRECT
@pragma selector_max_group_size 20
@pragma selector_num_max_groups SELECTOR_COUNT
#endif
table match_tbl {
    reads {
#ifdef MATCH_EXM
        ipv4.dstAddr : exact;
        ipv4.srcAddr : exact;
        ipv4.protocol : exact;
        ethernet.dstAddr : exact;
#elif defined(MATCH_TCAM)
        ipv4.dstAddr:ternary;
#elif defined(MATCH_ATCAM)
        ipv4.dstAddr : ternary;
        vlan_tag.vlan_id : exact;
#elif defined(MATCH_ALPM)
        ipv4.dstAddr : lpm;
        vlan_tag.vlan_id : exact;
#endif
    }
#ifdef ACTION_INDIRECT
    action_profile : selector_profile;
#elif defined(ACTION_DIRECT)
    actions {
        tcp_sport_modify;
        tcp_dport_modify;
        ipsa_modify;
        ipda_modify;
        ipds_modify;
        ipttl_modify;
    }
#endif
    size : MATCH_COUNT;
#ifdef SUPPORT_IDLE
    support_timeout: true;
#endif
}

/* Main control flow */
control ingress {
    apply(match_tbl);
}

control egress {
}

