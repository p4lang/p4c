#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"


header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
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
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type meta_t {
     fields {
         egress_ifindex : 16;         
         instance_id : 16;
         down_port : 1;
         from_packet_generator : 1;
         padding : 6;
     }
}


header ethernet_t ethernet;
header ipv4_t ipv4;
header tcp_t tcp;
metadata meta_t meta;


parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x0800 : parse_ipv4;
        default : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol){
       0x06 : parse_tcp;
       default : ingress;
    }
}

parser parse_tcp {
    extract(tcp);
    return ingress;
}

field_list lag_hash_fields {
    ethernet.srcAddr;
    ethernet.dstAddr;
    ipv4.srcAddr;
    ipv4.dstAddr;
    ipv4.protocol;
    tcp.srcPort;
    tcp.dstPort;
}

field_list_calculation lag_hash {
    input { lag_hash_fields; }
    algorithm : crc16;
    output_width : 14;
}

action_selector lag_selector {
    selection_key : lag_hash;
    /* selection_map : lag_mbrs; */
}
action set_lag_port(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action_profile lag_action_profile {
    actions {
        set_lag_port;
    }
    size : 1024;
    dynamic_action_selection : lag_selector;
}


register lag_mbrs {
    width : 1;
    instance_count : 1;
}

blackbox stateful_alu port_status_alu {
    reg: lag_mbrs;
    selector_binding: lag_group;
    update_lo_1_value: clr_bit;
}

action set_mbr_down(){
    port_status_alu.execute_stateful_alu();
}

action drop_packet(){
    drop();
}


table lag_group {
    reads {
        meta.egress_ifindex : exact;
    }
    action_profile: lag_action_profile;
}

table lag_group_fast_update {
    reads {
         meta.down_port : exact;
         meta.instance_id : exact;
    }
    actions {
         set_mbr_down;
         drop_packet;
    }
}

/* Main control flow */

control ingress {
    if (meta.from_packet_generator == 0){
        apply(lag_group);
    } else {
        apply(lag_group_fast_update);
    }
}
