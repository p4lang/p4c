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
         needs_sampling : 8;
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

register flow_cnt {
    width : 8;
    direct : ipv4_fib;
}

blackbox stateful_alu sampler_alu {
    reg: flow_cnt;
    condition_lo: register_lo == 100;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 0;

    update_lo_2_predicate: not condition_lo;
    update_lo_2_value: register_lo + 1;
    update_hi_1_value: 1;

    output_predicate: condition_lo;
    output_value : alu_hi;
    output_dst : meta.needs_sampling;   
}

action on_miss() { }

action drop_me(){
   drop();
}

action ipv4_fib_hit() {
    sampler_alu.execute_stateful_alu();
}

table ipv4_fib {
    reads {
        /* l3_metadata.vrf : exact; */
        ipv4.dstAddr : exact;
    }
    actions {
        ipv4_fib_hit;
    }
    default_action: on_miss;
    size : 1024;
}

table check_needs {
    reads {
        meta.needs_sampling : exact;
    }
    actions {
        drop_me;
        on_miss;
    }
}

/* Main control flow */

control ingress {
    apply(ipv4_fib);
    apply(check_needs);
}
