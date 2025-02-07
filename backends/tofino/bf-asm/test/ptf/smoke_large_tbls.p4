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

counter dummy_cntr {
   type : packets;
   direct : idle_stats_tbl;
}

action drop_count (count_idx) {
    //count(dummy_cntr, count_idx);
    drop();
}

action nop() {
}

action set_egress (egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

@pragma command_line --no-dead-code-elimination
table idle_stats_tbl {
    reads {
        ethernet.dstAddr : ternary;
        ethernet.srcAddr : ternary;
        ethernet.etherType : ternary;
        vlan_tag.pri : ternary;
        vlan_tag.cfi : ternary;
        vlan_tag.vlan_id : ternary;
        vlan_tag.etherType : ternary;
        ipv4.version : ternary;
        ipv4.ihl : ternary;
        ipv4.diffserv : ternary;
        ipv4.totalLen : ternary;
        ipv4.identification : ternary;
        ipv4.flags : ternary;
        ipv4.fragOffset : ternary;
        ipv4.ttl : ternary;
        ipv4.protocol : ternary;
        ipv4.hdrChecksum : ternary;
        ipv4.dstAddr : lpm;
        ipv4.srcAddr : ternary;
        tcp.srcPort : ternary;
        tcp.dstPort : ternary;
        tcp.seqNo : ternary;
        tcp.ackNo : ternary;
        tcp.dataOffset : ternary;
        tcp.res : ternary;
        tcp.ecn : ternary;
        tcp.ctrl : ternary;
        tcp.window : ternary;
        tcp.checksum : ternary;
        tcp.urgentPtr : ternary;
        udp.srcPort : ternary;
        udp.dstPort : ternary;
        udp.hdr_length : ternary;
    }
    actions {
        set_egress;
        nop;
    }
    size : 2048;
    support_timeout: true;
}

@pragma pack 2
@pragma ways 4 
@pragma atcam_partition_index vlan_tag.vlan_id
//@pragma atcam_number_partitions 1024 
table atcam_tbl {
    reads {
        tcp.valid : ternary;
        vlan_tag.vlan_id : exact;
        ipv4.dstAddr : ternary;
    }
    actions {
        set_egress;
        nop;
    }
    size: 500000;
}

action set_ip_id (ip_id, egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    modify_field(ipv4.identification, ip_id);
}

action_profile atcam_action_profile {
    actions {
        set_ip_id;
        nop;
    }
    size : 65536;
}

@pragma atcam_partition_index vlan_tag.vlan_id
//@pragma atcam_number_partitions 1024 
table atcam_indirect_tbl {
    reads {
        vlan_tag.vlan_id : exact;
        ipv4.srcAddr : ternary;
    }
    action_profile : atcam_action_profile;
    size: 100000;
}

control ingress {
    apply(idle_stats_tbl);
    apply(atcam_tbl);
    apply(atcam_indirect_tbl);
}

control egress {
}

