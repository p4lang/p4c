#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
        blah : 16;

        x : 32;
        y : 32;
        z : 32;
    }
}

header ethernet_t ethernet;


parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action action_0(){
    sampling_alu.execute_stateful_alu(8191);
}

action action_1(){
}

register sampling_cntr {
    width : 32;
    static: table_0;
    instance_count : 8192;
}

blackbox stateful_alu sampling_alu {
    reg: sampling_cntr;
    condition_lo: 1;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: register_lo + 1;
    output_predicate: condition_lo;
    output_value : alu_lo;
    output_dst : ethernet.x;
}

@pragma ways 1
table table_0 {
    reads {
        ethernet.blah: exact;
    }
    actions {
        action_0;
        //action_1;
    }
    size : 1024;
}

control ingress { 
    apply(table_0);
}
