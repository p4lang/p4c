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

meter exm_meter1 {
    type : bytes;
    static : exm_tbl_act_spec_format_1;
    result : ipv4.diffserv;
    instance_count : 500;
}

counter exm_cntr1 {
    type : packets;
    direct : exm_tbl_act_spec_format_1;
}

meter exm_meter2 {
    type : bytes;
    direct : exm_tbl_act_spec_format_2;
    result : ipv4.diffserv;
}

counter exm_cntr2 {
    type : packets;
    static : exm_tbl_act_spec_format_2;
    instance_count : 500;
}

meter exm_meter3 {
    type : bytes;
    static : exm_tbl_act_spec_format_3;
    result : ipv4.diffserv;
    instance_count : 500;
}

counter exm_cntr3 {
    type : packets;
    static : exm_tbl_act_spec_format_3;
    instance_count : 500;
}

//meter exm_meter4 {
//    type : bytes;
//    direct : exm_tbl_act_spec_format_4;
//    result : ipv4.diffserv;
//}
//
//counter exm_cntr4 {
//    type : packets;
//    direct : exm_tbl_act_spec_format_4;
//}

meter tcam_meter1 {
    type : bytes;
    static : tcam_tbl_act_spec_format_1;
    result : ipv4.diffserv;
    instance_count : 500;
}

counter tcam_cntr1 {
    type : packets;
    direct : tcam_tbl_act_spec_format_1;
}

meter tcam_meter2 {
    type : bytes;
    direct : tcam_tbl_act_spec_format_2;
    result : ipv4.diffserv;
}

counter tcam_cntr2 {
    type : packets;
    static : tcam_tbl_act_spec_format_2;
    instance_count : 500;
}

meter tcam_meter3 {
    type : bytes;
    static : tcam_tbl_act_spec_format_3;
    result : ipv4.diffserv;
    instance_count : 500;
}

counter tcam_cntr3 {
    type : packets;
    static : tcam_tbl_act_spec_format_3;
    instance_count : 500;
}

//meter tcam_meter4 {
//    type : bytes;
//    direct : tcam_tbl_act_spec_format_4;
//    result : ipv4.diffserv;
//}
//
//counter tcam_cntr4 {
//    type : packets;
//    direct : tcam_tbl_act_spec_format_4;
//}

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

action next_hop_ipv4_meters_1(egress_port ,srcmac, dstmac, meter_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    execute_meter(exm_meter1, meter_idx, ipv4.diffserv);
}

action next_hop_ipv4_stats_2(egress_port ,srcmac, dstmac, stat_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    count(exm_cntr2, stat_idx);
}

action next_hop_ipv4_stats_meters_3(egress_port ,srcmac, dstmac, meter_idx, stat_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    count(exm_cntr3, stat_idx);
    execute_meter(exm_meter3, meter_idx, ipv4.diffserv);
}

action next_hop_ipv4_stats_3(egress_port ,srcmac, dstmac, stat_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    count(exm_cntr3, stat_idx);
}

action next_hop_ipv4_meters_3(egress_port ,srcmac, dstmac, meter_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    execute_meter(exm_meter3, meter_idx, ipv4.diffserv);
}

action tcam_next_hop_ipv4_meters_1(egress_port ,srcmac, dstmac, meter_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    execute_meter(tcam_meter1, meter_idx, ipv4.diffserv);
}

action tcam_next_hop_ipv4_stats_2(egress_port ,srcmac, dstmac, stat_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    count(tcam_cntr2, stat_idx);
}

action tcam_next_hop_ipv4_stats_meters_3(egress_port ,srcmac, dstmac, meter_idx, stat_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    count(tcam_cntr3, stat_idx);
    execute_meter(tcam_meter3, meter_idx, ipv4.diffserv);
}

action tcam_next_hop_ipv4_stats_3(egress_port ,srcmac, dstmac, stat_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    count(tcam_cntr3, stat_idx);
}

action tcam_next_hop_ipv4_meters_3(egress_port ,srcmac, dstmac, meter_idx) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
    execute_meter(tcam_meter3, meter_idx, ipv4.diffserv);
}

/* Exact match table referring to meter table indirectly for one action
 * And directly referring to a stats table.
 */

table exm_tbl_act_spec_format_1 {
    reads {
        ipv4.srcAddr : exact;
        ipv4.dstAddr : exact;
    }
    actions {
        next_hop_ipv4_meters_1;
        next_hop_ipv4;
    }
    default_action: nop();
}

/* Exact match table referring to stat table indirectly for one action
 * And directly referring to a meter table
 */

table exm_tbl_act_spec_format_2 {
    reads {
        ipv4.srcAddr : exact;
        ipv4.dstAddr : exact;
    }
    actions {
        next_hop_ipv4_stats_2;
        next_hop_ipv4;
    }
    default_action: nop();
}

/* Exact match table referring to stat table and meter table indirectly with
 * all combinations across four actions.
 */

table exm_tbl_act_spec_format_3 {
    reads {
        ipv4.srcAddr : exact;
        ipv4.dstAddr : exact;
    }
    actions {
        next_hop_ipv4_stats_meters_3;
        next_hop_ipv4_stats_3;
        next_hop_ipv4_meters_3;
        next_hop_ipv4;
    }
    default_action: nop();
}

/* Exact match table referring to a stat table and meter table directly */

table exm_tbl_act_spec_format_4 {
    reads {
        ipv4.srcAddr : exact;
        ipv4.dstAddr : exact;
    }
    actions {
        next_hop_ipv4;
    }
    default_action: nop();
}

/* TCAM table referring to meter table indirectly for one action
 * And directly referring to a stats table.
 */

table tcam_tbl_act_spec_format_1 {
    reads {
        ipv4.srcAddr : lpm;
        ipv4.dstAddr : exact;
    }
    actions {
        tcam_next_hop_ipv4_meters_1;
        next_hop_ipv4;
    }
    default_action: nop();
}

/* TCAM table referring to stat table indirectly for one action
 * And directly referring to a meter table
 */

table tcam_tbl_act_spec_format_2 {
    reads {
        ipv4.srcAddr : lpm;
        ipv4.dstAddr : exact;
    }
    actions {
        tcam_next_hop_ipv4_stats_2;
        next_hop_ipv4;
    }
    default_action: nop();
}

/* TCAM table referring to stat table and meter table indirectly with
 * all combinations across four actions.
 */

table tcam_tbl_act_spec_format_3 {
    reads {
        ipv4.srcAddr : lpm;
        ipv4.dstAddr : exact;
    }
    actions {
        tcam_next_hop_ipv4_stats_meters_3;
        tcam_next_hop_ipv4_stats_3;
        tcam_next_hop_ipv4_meters_3;
        next_hop_ipv4;
    }
    default_action: nop();
}

/* TCAM table referring to a stat table and meter table directly */

table tcam_tbl_act_spec_format_4 {
    reads {
        ipv4.srcAddr : lpm;
        ipv4.dstAddr : exact;
    }
    actions {
        next_hop_ipv4;
        nop;
    }
    default_action: nop();
}


control ingress {
    apply(exm_tbl_act_spec_format_2);
    apply(tcam_tbl_act_spec_format_2);
    apply(exm_tbl_act_spec_format_1);
    apply(exm_tbl_act_spec_format_3);
    apply(tcam_tbl_act_spec_format_1);
    apply(tcam_tbl_act_spec_format_3);
    //apply(exm_tbl_act_spec_format_4);
}

control egress {
}
