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

        blah1 : 32;
        blah2 : 8;
        blah3 : 64;
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


field_list first_field_list {
    ipv4.blah1;
    ipv4.blah2;
    ipv4.blah3;
}


field_list_calculation first_hash {
   input {
      first_field_list;
   }
   algorithm : random;
   output_width : 72;
}


action big_action(param0, param1, param2/*, param3*/){
    modify_field(ipv4.dstAddr, param0);
    modify_field(ipv4.srcAddr, param1);
    modify_field(ethernet.dstAddr, param2);
    //modify_field(ethernet.srcAddr, param3);
}

action action_0(param0){
    modify_field(ipv4.hdrChecksum, param0);
}

action action_select(base, hash_size){
    modify_field_with_hash_based_offset(ipv4.blah2, base, first_hash, hash_size);
}

action do_nothing(){
    no_op();
}

@pragma immediate 0
@pragma selector_max_group_size 121
table test_select {
    reads {
        ethernet.etherType : exact;
        ethernet.dstAddr : exact;
        ethernet.srcAddr : exact;
    }
    action_profile : some_action_profile;
    size : 8192;
}

action_profile some_action_profile {
    actions {
       action_0;
       big_action;
       do_nothing;
    }
    size : 512;
    dynamic_action_selection : some_selector ;
}

action_selector some_selector {
    selection_key : first_hash;
    selection_mode : resilient;
}
    


/*
table table_select {
   reads {
       ethernet.etherType : exact;
   }
   actions {
       do_nothing;
       action_0;
       //big_action;
   }
   max_size : 2048;
}
*/

table table_group {
   reads {
       ipv4.blah1 : ternary;
   }
   actions {
       action_select;
   }
}


/* Main control flow */

control ingress {

      apply(test_select);

      apply(table_group);
//    apply(table_select);

}
