#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"


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

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type meta_t {
     fields {
         pad_1 : 6;
         hash_1 : 18;
         pad_2 : 6;
         hash_2 : 18;
         pad_3 : 6;
         hash_3 : 18;

         pad_4 : 7;
         is_not_member : 1;
     }
}

header ethernet_t ethernet;
header ipv4_t ipv4;
header tcp_t tcp;
metadata meta_t meta;

parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x0800 : parse_ipv4;
        default : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol){
       0x06 : parse_tcp;
       default : ingress;
    }
}

parser parse_tcp {
    extract(tcp);
    return ingress;
}

field_list fields_for_hash {
    ipv4.srcAddr;
    ipv4.dstAddr;
}

field_list_calculation hash_1 {
    input { fields_for_hash; }
    algorithm: random;
    output_width: 18;
}

field_list_calculation hash_2 {
    input { fields_for_hash; }
    algorithm: crc32;
    output_width: 18;
}

field_list_calculation hash_3 {
    input { fields_for_hash; }
    algorithm: identity;
    output_width: 18;
}


register bloom_filter_1 {
    width : 1;
    direct : bloom_filter_membership_1; 
}

register bloom_filter_2 {
    width : 1;
    direct : bloom_filter_membership_2; 
}

register bloom_filter_3 {
    width : 1;
    direct : bloom_filter_membership_3; 
}

blackbox stateful_alu bloom_filter_alu_1 {
    reg: bloom_filter_1;

    update_lo_1_value: set_bitc;

    output_value: alu_lo;
    output_dst: meta.is_not_member;  /* Want reduction OR */
}

blackbox stateful_alu bloom_filter_alu_2 {
    reg: bloom_filter_2;

    update_lo_1_value: set_bitc;

    output_value: alu_lo;
    output_dst: meta.is_not_member;  /* Want reduction OR */
}

blackbox stateful_alu bloom_filter_alu_3 {
    reg: bloom_filter_3;

    update_lo_1_value: set_bitc;

    output_value: alu_lo;
    output_dst: meta.is_not_member;  /* Want reduction OR */
}

action run_bloom_filter_1(){
    bloom_filter_alu_1.execute_stateful_alu();
}
action run_bloom_filter_2(){
    bloom_filter_alu_2.execute_stateful_alu();
}
action run_bloom_filter_3(){
    bloom_filter_alu_3.execute_stateful_alu();
}

action set_hash_1(){
    modify_field_with_hash_based_offset(meta.hash_1, 0, hash_1, 262144);
}
action set_hash_2(){
    modify_field_with_hash_based_offset(meta.hash_2, 0, hash_2, 262144);
}
action set_hash_3(){
    modify_field_with_hash_based_offset(meta.hash_3, 0, hash_3, 262144);
}




action drop_me(){
    drop();
}

action do_nothing(){}


table set_hash_1_tbl {
    actions {
        set_hash_1;
    }
    size : 256;
}

table set_hash_2_tbl {
    actions {
        set_hash_2;
    }
    size : 1;
}

table set_hash_3_tbl {
    actions {
        set_hash_3;
    }
    size : 1;
}

table bloom_filter_membership_1 {
   reads {
      meta.hash_1 : exact;
   }
   actions {
      run_bloom_filter_1;
   }
   size : 262144;
}

table bloom_filter_membership_2 {
   reads {
      meta.hash_2 : exact;
   }
   actions {
      run_bloom_filter_2;
   }
   size : 262144;
}

table bloom_filter_membership_3 {
   reads {
      meta.hash_3 : exact;
   }
   actions {
      run_bloom_filter_3;
   }
   size : 262144;
}

table react {
    reads {
       meta.is_not_member : exact;
    }
    actions {
       drop_me;
       do_nothing;
    }
}

/* Main control flow */

control ingress {
    apply(set_hash_1_tbl);
    apply(set_hash_2_tbl);
    apply(set_hash_3_tbl);

    apply(bloom_filter_membership_1);
    apply(bloom_filter_membership_2);
    apply(bloom_filter_membership_3);

    apply(react);
}
