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
         is_not_heavy_hitter : 1;
         padding : 7;
         
         pad_1 : 4;
         hash_1 : 12;
         pad_2 : 4;
         hash_2 : 12;
         //pad_3 : 4;
         hash_3 : 32;
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

field_list fields_for_hash { ipv4.srcAddr; }

field_list_calculation hash_1 {
   input { fields_for_hash; }
   algorithm : crc16;
   output_width : 12;
}

field_list_calculation hash_2 {
   input { fields_for_hash; }
   algorithm : crc16_msb;
   output_width : 12;
}

field_list_calculation hash_3 {
   input { fields_for_hash; }
   algorithm : identity;
   output_width : 32;
}

register sketch_cnt_1 {
    width: 32;
    direct: heavy_hitter_tbl_1;
}

register sketch_cnt_2 {
    width: 32;
    direct: heavy_hitter_tbl_2;
}

register sketch_cnt_3 {
    width: 32;
    direct: heavy_hitter_tbl_3;
}

blackbox stateful_alu sketch_cnt_alu_1 {
    reg: sketch_cnt_1;

    condition_hi: register_lo > 10000000;
    condition_lo: register_lo < 50000000;
  
    update_lo_1_value: register_lo + 1;
    update_hi_1_value: 1;

    output_predicate: not condition_hi or not condition_lo;
    output_value: alu_hi;
    output_dst: meta.is_not_heavy_hitter;  /* TODO: make reduction or */
}

blackbox stateful_alu sketch_cnt_alu_2 {
    reg: sketch_cnt_2;

    condition_hi: register_lo > 10000000;
    condition_lo: register_lo < 50000000;
  
    update_lo_1_value: register_lo + 1;
    update_hi_1_value: 1;

    output_predicate: not condition_hi or not condition_lo;
    output_value: alu_hi;
    output_dst: meta.is_not_heavy_hitter;  /* TODO: make reduction or */
}

blackbox stateful_alu sketch_cnt_alu_3 {
    reg: sketch_cnt_3;

    condition_hi: register_lo > 10000000;
    condition_lo: register_lo < 50000000;
  
    update_lo_1_value: register_lo + 1;
    update_hi_1_value: 1;

    output_predicate: not condition_hi or not condition_lo;
    output_value: alu_hi;
    output_dst: meta.is_not_heavy_hitter;  /* TODO: make reduction or */
}

action run_alu_1(){
    sketch_cnt_alu_1.execute_stateful_alu();
}

action run_alu_2(){
    sketch_cnt_alu_2.execute_stateful_alu();
}

action run_alu_3(){
    sketch_cnt_alu_3.execute_stateful_alu();
}

action set_hash_1_and_2(){
    modify_field_with_hash_based_offset(meta.hash_1, 0, hash_1, 4096);
    modify_field_with_hash_based_offset(meta.hash_2, 0, hash_2, 4096);
}

action set_hash_3(){
    modify_field_with_hash_based_offset(meta.hash_3, 0, hash_3, 4294967296);//4096);
}

action drop_me(){
    drop();
}

action do_nothing(){}


table set_hashes_1_and_2_tbl {
    actions {
        set_hash_1_and_2;
    }
    size : 256;
}

table set_hash_3_tbl {
    actions {
        set_hash_3;
    }
    size : 256;
}

table heavy_hitter_tbl_1 {
    reads {
        meta.hash_1 : exact;
    }
    actions {
        run_alu_1;
    }
}

table heavy_hitter_tbl_2 {
    reads {
        meta.hash_2 : exact;
    }
    actions {
        run_alu_2;
    }
}

table heavy_hitter_tbl_3 {
    reads {
        meta.hash_3 : exact;
    }
    actions {
        run_alu_3;
    }
}


table react {
    reads {
        meta.is_not_heavy_hitter : exact;
    }
    actions {
        drop_me;
        do_nothing;
    }
}


/* Main control flow */

control ingress {
   apply(set_hashes_1_and_2_tbl);
   apply(set_hash_3_tbl);

   apply(heavy_hitter_tbl_1);
   apply(heavy_hitter_tbl_2);
   apply(heavy_hitter_tbl_3);

   apply(react);
}
