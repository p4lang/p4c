#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_32 : 32;
        field_b_32 : 32;
        field_c_32 : 32;
        field_d_32 : 32;

        field_e_16 : 16;
        field_f_16 : 16;
        field_g_16 : 16;
        field_h_16 : 16;

        field_i_8 : 8;
        field_j_8 : 8;
        field_k_8 : 8;
        field_l_8 : 8;

        field_m_32 : 32;
        field_n_32 : 32;
    }
}


parser start {
    return parse_pkt;
}

header pkt_t pkt;

parser parse_pkt {
    extract(pkt);
    return ingress;
}

action action_0(param0){
    modify_field(pkt.field_f_16, param0);
}

action action_1(param0){
    modify_field(pkt.field_f_16, param0);
}

action action_2(param0){
    modify_field(pkt.field_f_16, param0);
}

action do_nothing(){
    no_op();
}

action do_nothing_1(){ 
}

table table_0 {
    reads {
         pkt.field_a_32  : exact;
    }
    actions {
        do_nothing;
        action_0;
    }
    size : 4096;
}

table table_1 {
    reads {
         pkt.field_b_32  : exact;
    }
    actions {
        do_nothing_1;
        action_1;
    }
    size : 4096;
}

table table_2 {
    reads {
         pkt.field_b_32  : exact;
    }
    actions {
        do_nothing;
        action_2;
    }
    size : 4096;
}


/* Main control flow */

control ingress {
    apply(table_0){
        do_nothing {
            if (valid(pkt)){
                apply(table_1){
                     do_nothing_1 {
                         if(valid(pkt)){
                             apply(table_2);
                         }
                     }
                }                
            }
        }
    }
}


