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
         next_hop : 8;
         dst_tor_id : 8;
         is_conga_packet : 1 ;
         padding : 7;
     }
}

header_type conga_meta_t {
     fields {
         next_hop : 8;
         util : 8;
     }
}

header_type conga_state_layout {
    fields {
       next_hop : 8;
       utilization : 8;
    }
}


header ethernet_t ethernet;
header ipv4_t ipv4;
header tcp_t tcp;
metadata meta_t meta;
metadata conga_meta_t conga_meta;


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



register conga_state {
    layout : conga_state_layout;
    instance_count : 256;
}

blackbox stateful_alu conga_alu {
    reg: conga_state;
    update_lo_1_value: register_hi;
    output_value: alu_lo;
    output_dst: meta.next_hop;
}

blackbox stateful_alu conga_update_alu {
    reg: conga_state;

    condition_hi: register_lo > conga_meta.util;
    condition_lo: register_hi == conga_meta.next_hop;
  
    update_hi_1_predicate: condition_hi;
    update_hi_1_value: conga_meta.next_hop;
    
    update_lo_1_predicate: condition_hi or condition_lo;
    update_lo_1_value: conga_meta.util;

    output_value: alu_hi;
    output_dst: meta.next_hop;
}

action get_preferred_next_hop(){
    conga_alu.execute_stateful_alu();
}

action update_preferred_next_hop(){
    conga_update_alu.execute_stateful_alu();
}


table conga_rd_next_hop_table {
    reads {
        meta.dst_tor_id : exact;
    }
    actions {
        get_preferred_next_hop;
    }
    size : 256;
}

table conga_wr_next_hop_table {
    reads {
        meta.dst_tor_id : exact;
        meta.next_hop : exact;   /* HACK */
        conga_meta.next_hop : exact;   /* HACK */
        conga_meta.util : exact;   /* HACK */
    }
    actions {
        update_preferred_next_hop;
    }
    size : 256;
}



/* Main control flow */

control ingress {
    if (meta.is_conga_packet == 1){
        apply(conga_wr_next_hop_table);
    } else {
        apply(conga_rd_next_hop_table);
    }

}
