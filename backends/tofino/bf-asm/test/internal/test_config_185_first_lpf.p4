#include "tofino/intrinsic_metadata.p4"
#include "tofino/lpf_blackbox.p4"
#include "tofino/wred_blackbox.p4"
#include "tofino/meter_blackbox.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
        blah0 : 16;
        blah1 : 8;
        blah2 : 8;
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
    my_lpf.execute(ethernet.blah0);
}

action do_nothing(){}

action action_1(){
    my_wred.execute(ethernet.blah1);
}

action action_2(){
    my_meter.execute(ethernet.blah2);
}

blackbox lpf my_lpf {
    filter_input : ethernet.etherType;
    direct : table_0;
}    

blackbox wred my_wred {
    wred_input : ethernet.etherType;
    direct : table_1;
}

blackbox meter my_meter {
    type : bytes;
    direct : table_2;
}

table table_0 {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        action_0;
        //do_nothing;
    }
    size : 1024;
}

table table_1 {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        action_1;
    }
    size : 1024;
}

table table_2 {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        action_2;
    }
    size : 1024;
}

control ingress { 
    apply(table_0);
    apply(table_1);
    apply(table_2);
}
