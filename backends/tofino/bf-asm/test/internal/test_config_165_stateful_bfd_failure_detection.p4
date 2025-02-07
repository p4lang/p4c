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
         bfd_timeout_detected : 8;
         bfd_tx_or_rx : 8;
         bfd_discriminator : 16;
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

register bfd_cnt {
    width : 8;
    /* direct : bfd; */
    instance_count : 1024;
}

blackbox stateful_alu bfd_cnt_rx_alu {
    reg: bfd_cnt;
    update_lo_1_value : 0;
}

blackbox stateful_alu bfd_cnt_tx_alu {
    reg: bfd_cnt;
    condition_lo: register_lo > 3;

    update_hi_1_value : 1;
    update_lo_1_value : register_lo + 1;

    output_predicate: condition_lo;
    output_value : alu_hi;
    output_dst : meta.bfd_timeout_detected;   
}

action on_miss() { }

action drop_me(){
   drop();
}

action bfd_rx() {
   bfd_cnt_rx_alu.execute_stateful_alu();
}

action bfd_tx() {
   bfd_cnt_tx_alu.execute_stateful_alu();
}


table bfd {
    reads {
        meta.bfd_tx_or_rx : exact;
        meta.bfd_discriminator : exact;
    }
    actions {
        bfd_rx;
        bfd_tx;
    }
    size : 1024;
}

table check_needs {
    reads {
        meta.bfd_timeout_detected : exact;
    }
    actions {
        drop_me;
        on_miss;
    }
}

/* Main control flow */

control ingress {
    apply(bfd);
    apply(check_needs);
}
