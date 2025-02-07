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

action action_0(param0){
    modify_field(ipv4.diffserv, param0);
}

action action_1(param1){
    modify_field(ipv4.hdrChecksum, param1);
}

action action_e(param2, param_3){
    modify_field(ipv4.diffserv, param2);
    modify_field(ipv4.hdrChecksum, param_3);
}

table table_0 {
   reads {
    ipv4.dstAddr: lpm;
   }       
   actions {
     action_0;
     action_1;
   }
   //max_size : 32768;
}

table table_1 {
    reads {
      ipv4.dstAddr: exact;
      ipv4.protocol: exact;
    }
    actions {
      action_1;
    }
}

table table_e {
   reads {
      ipv4.srcAddr: exact;
      ipv4.diffserv: exact;
   }
   actions {
      action_e;
   }
}

/* Main control flow */

control ingress {
    apply(table_0);
    apply(table_1);
}


control egress {
    apply(table_e);
}
