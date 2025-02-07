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

action action_0(){
    modify_field(ipv4.diffserv, 1);
}

action action_1() {
    modify_field(ipv4.totalLen, 2);
}

action action_2() {
    modify_field(ipv4.identification, 3);
}

action action_3(param_3) {
    modify_field(ipv4.ttl, param_3);
}

action action_4(param_4) {
    modify_field(ipv4.protocol, param_4);
}

action do_nothing(){
    no_op();
}


table table_0 {
   reads {
     ethernet.etherType : ternary;
     ethernet.srcAddr mask 0xffffffff00: ternary;
   }       
   actions {
     action_0;
     do_nothing;
   }
   max_size : 1024;
}

@pragma ways 6
@pragma pack 3
table table_1 {
   reads {
     ipv4.srcAddr : exact;
     ipv4.dstAddr : exact;
     ipv4.fragOffset : exact;
     ethernet.srcAddr : exact;
     ethernet.dstAddr : exact;
     //ipv4.dstAddr mask 0xffff0000 : ternary;
     //ethernet.srcAddr : ternary;
     //ethernet.dstAddr : ternary;
   }       
   actions {
     action_1;
     do_nothing;
   }
   max_size : 40960;
}

/* Main control flow */

control ingress {
    apply(table_0);
    apply(table_1);
}
