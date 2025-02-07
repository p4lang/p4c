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

action action_0(){
    modify_field(ipv4.diffserv, 1);
}

action action_1() {
    modify_field(ipv4.totalLen, 2);
}

action action_2() {
    modify_field(ipv4.identification, 3);
}

action action_3(){
    modify_field(ipv4.identification, 4);
}


action do_nothing(){
    no_op();
}


table table_0 {
   reads {
    ethernet.etherType : lpm;
    ipv4.diffserv : exact;
   }       
   actions {
     action_0;
     do_nothing;
   }
   max_size : 1024;
}

table table_1 {
   reads {
     ipv4.srcAddr : exact;
     ipv4.dstAddr : exact;
   }       
   actions {
     action_1;
     do_nothing;
   }
   max_size : 16384;
}

table table_2 {
   reads {
     ipv4.srcAddr : exact;
     ipv4.totalLen : exact;
   }       
   actions {
     action_2;
     do_nothing;
   }
   max_size : 4096;
}

table table_3 {
   reads {
      ipv4.srcAddr : exact;
   }
   actions {
      action_3;
   }
   max_size : 2048;
}


/* Main control flow */

control ingress {

    apply(table_0);
    apply(table_1);

    if (valid(ipv4)){
       apply(table_2);
    }

    apply(table_3);
}
