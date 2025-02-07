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
         sflow_sample_seq_no : 32;
         sflow_src : 16;
     }
}

header_type sflowHdr_t {
      fields {
          seq_num : 16;
          num_samples : 16;
          temp : 16 (saturating);
          drops : 16;
      }
}


header ethernet_t ethernet;
header ipv4_t ipv4;
header tcp_t tcp;
metadata meta_t meta;
metadata sflowHdr_t sflowHdr;

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


register sflow_state_seq_num {
    width : 32;
    direct : sflow_seq_num;
}

blackbox stateful_alu seq_num_gen {
    reg: sflow_state_seq_num;

    update_lo_1_value: register_lo + 1;
    update_hi_1_value: register_lo;

    output_value: alu_hi;
    output_dst: meta.sflow_sample_seq_no;
}


register sflow_state_exp_seq_num {
    width : 32;
    direct : sflow_verify_seq_no_step_2;
}

blackbox stateful_alu sflow_exp_seq_num {
    reg: sflow_state_exp_seq_num;

    update_hi_1_value: sflowHdr.seq_num - register_lo;
    update_lo_1_value: sflowHdr.temp;

    output_value: alu_hi;
    output_dst: sflowHdr.drops;
}


action get_sflow_seq_num(){
    seq_num_gen.execute_stateful_alu();
}

action calc_next_seq_num() {
    add(sflowHdr.temp, sflowHdr.seq_num, sflowHdr.num_samples);
}

action chk_sflow_seq_num() {
    sflow_exp_seq_num.execute_stateful_alu();
}

action drop_me(){
    drop();
}

action do_nothing(){}


table sflow_seq_num {
    reads { 
        meta.sflow_src: exact;
        meta.sflow_sample_seq_no : exact;  /* HACK */
    } 
    actions {
        get_sflow_seq_num;
    }
    size : 1024;
}

table sflow_verify_seq_no_step_1 {
     actions {
        calc_next_seq_num;
    }
    size : 512;
}

@pragma stage 1  /* Provided dependency graph is incorrect */
table sflow_verify_seq_no_step_2 {
    reads {
        meta.sflow_src : exact;
    }
    actions {
        chk_sflow_seq_num;
    }
    size : 16384;
}

table sflow_verify_seq_no_step_3 {
    reads {
        sflowHdr.drops : ternary;
    }
    actions {
        drop_me;
        do_nothing;
    }
    size : 512;
}



/* Main control flow */

control ingress {
    apply(sflow_seq_num);
    apply(sflow_verify_seq_no_step_1);
    apply(sflow_verify_seq_no_step_2);
    apply(sflow_verify_seq_no_step_3);
}
