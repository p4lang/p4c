#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
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

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        0x800 : parse_ipv4;
        default: ingress;
    }
}

header ipv4_t ipv4;

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        drop: 1;
    }
}

metadata routing_metadata_t /*metadata*/ routing_metadata;

action action_0(param_0){
    modify_field(ipv4.diffserv, param_0);
}

action action_1(){
    modify_field(ipv4.totalLen, 1);
}

table table_0 {
   reads {
     ethernet.etherType : ternary;
   }       
   actions {
     action_0;
   }
   max_size : 4096;
}

table table_1 {
   reads {
     ethernet.etherType : ternary;
   }       
   actions {
     action_1;
   }
   max_size : 4096;
}

/* Main control flow */

control ingress {
    apply(table_0);
    apply(table_1);
}

