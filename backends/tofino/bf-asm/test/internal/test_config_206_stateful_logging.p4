#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
        blah : 16;
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

register logging_reg {
    width : 16;
    static: table_0;
    instance_count : 16384; 
}

blackbox stateful_alu logging_alu {
    reg: logging_reg;
    update_lo_1_value: ethernet.blah;
    stateful_logging_mode : table_hit;
}

action action_0(){
   drop();
   logging_alu.execute_stateful_log();
}

action action_1(){ }  /* Note that stateful logging is still performed here. */

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


control ingress { 
    apply(table_0);
}
