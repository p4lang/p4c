#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"

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

        field_m_16 : 16;

    }
}

parser start {
    return parse_ethernet;
}

header pkt_t pkt;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

action do_nothing(){}

action action_0(param_0) {
    bit_xor(pkt.field_f_16, pkt.field_g_16, param_0); 
    modify_field(pkt.field_e_16, param_0);
}

@pragma immediate 0
table table_0 {
    reads {
         pkt.field_l_8 : ternary;
         
         pkt.field_a_32 : ternary;
         pkt.field_b_32 : exact;
         pkt.field_c_32 : exact;
         pkt.field_e_16 : exact;
 
    }
    actions {
        action_0;
        drop_me;
        do_nothing;
    }
    size : 256;
}

action action_1(param_1){
    bit_xor(pkt.field_a_32, pkt.field_a_32, param_1);
    modify_field(pkt.field_b_32, param_1);
}


@pragma immediate 0
table table_1 {
    reads {
         pkt.field_a_32 mask 0xff: ternary;
    }
    actions {
        action_1;
    }
    size : 256;
}

table table_2 {
    reads {
        pkt.field_b_32 : exact;
    }
    actions {
        action_1;
        drop_me;
    }
}

action drop_me(){
   drop();
}

table table_e {
    reads { 
         pkt.field_a_32: ternary;
    }
    actions {
        do_nothing;
        drop_me;
    }
    size : 512;
}

control ingress {

    apply(table_0) {
        hit {
             apply(table_1);
             apply(table_2);
        }
    }
    apply(table_e);    
}

control egress {
}
