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

header_type meta_t {
     fields {
         encap_decap_size : 16;
     }
}

header ethernet_t ethernet;
header ipv4_t ipv4;
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
    return ingress;
}

register cntr {
    width : 64;
    instance_count : 8192;
}

blackbox stateful_alu counter_alu {
    reg: cntr;
    condition_hi: register_lo < 0;
    condition_lo: register_lo + meta.encap_decap_size < 0;

    update_hi_1_predicate: condition_hi and not condition_lo;
    update_hi_1_value: register_hi + 1;
    
    update_hi_2_predicate: not condition_hi and condition_lo;
    update_hi_2_value: register_hi - 1;

    update_lo_1_value : register_lo + meta.encap_decap_size;
}

action increment_counter() {
   counter_alu.execute_stateful_alu();
}

table packet_offset_counting {
    reads {
        meta.encap_decap_size : exact;  /* hack to get allocated */
        ig_intr_md.ingress_port : exact;
    }
    actions {
        increment_counter;
    }
    size : 1024;
}

/* Main control flow */

control ingress {
    apply(packet_offset_counting);
}
