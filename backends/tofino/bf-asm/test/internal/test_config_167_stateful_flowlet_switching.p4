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

header_type flowlet_state_layout {
    fields {
       padding : 16;
       flowlet_next_hop : 16;
       flowlet_last_time_seen : 32;
    }
}

header_type meta_t {
     fields {
         next_hop : 16;
         tstamp : 32;
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

field_list hash_fields_5_tuple {
    ipv4.srcAddr;
    ipv4.dstAddr;
    ipv4.protocol;
    tcp.srcPort;
    tcp.dstPort;
}

field_list_calculation hash_id {
    input { hash_fields_5_tuple; }
    algorithm : crc32;
    output_width : 16;
}


register flowlet_state {
    layout : flowlet_state_layout;
    instance_count : 65536;
}

blackbox stateful_alu flowlet_state_alu {
    reg: flowlet_state;

    condition_lo: meta.tstamp - register_lo > 20000;

    update_hi_1_predicate: condition_lo;
    update_hi_1_value: register_hi;
    
    update_lo_1_value: meta.tstamp;

    output_value: alu_hi;
    output_dst: meta.next_hop;
}

action get_flowlet_next_hop() {
   flowlet_state_alu.execute_stateful_alu();
}

table flowlet_next_hop {
    reads { /* HACK, because actually want to use hash result to index */
        meta.next_hop : ternary;
        meta.tstamp: exact;
    } 
    actions {
        get_flowlet_next_hop;
    }
    size : 1024;
}

/* Main control flow */

control ingress {
    apply(flowlet_next_hop);
}
