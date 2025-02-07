#include "tofino/intrinsic_metadata.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type meta_t {
     fields {
         a : 16;
         b : 16;
         c : 16;
         d : 16;
     }
}


header ethernet_t ethernet;
metadata meta_t meta;


parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action action_0(){
    bypass_egress();
    //mark_for_drop();
}

action action_1(){
   drop();
}

action action_2(){
   //exit();
}

table table_0 {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        action_0;
        action_1;
    }
    size : 1024;
}

table table_1 {
    reads { 
       ethernet.dstAddr: exact;
    }
    actions {
       action_2;  
    }
}

table table_2 {
    reads { 
       ethernet.dstAddr: exact;
    }
    actions {
       action_0;  
    }
}

control ingress {
    apply(table_0); 
    apply(table_1);
    apply(table_2);
}
