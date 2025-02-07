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

action action_0(){
    sampling_alu.execute_stateful_alu();
}

action action_1(){
}


register sampling_cntr {
    width : 32;
    static: table_0;
    instance_count : 139264; /* Fills 34 + 1 spare RAMs (max size - 1) */
}

/* Note the extra complexity of this ALU program is required so that if C2C was
 * already set (by the bloom filter) it will stay set even if this ALU says not
 * to sample. */
blackbox stateful_alu sampling_alu {
    reg: sampling_cntr;
    initial_register_lo_value: 1;
    condition_lo: register_lo >= 10;
    condition_hi: ig_intr_md_for_tm.copy_to_cpu != 0;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 1;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value: register_lo + 1;
    update_hi_1_value: 1;
    output_predicate: condition_lo or condition_hi;
    output_value : alu_hi;
    output_dst : ig_intr_md_for_tm.copy_to_cpu;
}

field_list bf_hash_fields {
    ethernet.dstAddr;
    ethernet.etherType;
}    

field_list_calculation bf_hash_1 {
    input { bf_hash_fields; }
    algorithm: random;
    output_width: 16;
}

action action_7(){
    modify_field_with_hash_based_offset(ethernet.blah, 0, bf_hash_1, 262144);
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
    actions { action_7; }
}

control ingress { 
    apply(table_0);
    apply(table_1);
}
