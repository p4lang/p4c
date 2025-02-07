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
        color_0 : 8;
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


action action_a(param0, param1, param2){
    modify_field(pkt.field_e_16, param0);
    modify_field(pkt.field_f_16, param1);
    modify_field(pkt.field_g_16, param2);
}

action action_b(param0){
    modify_field(pkt.field_f_16, param0);
}

action action_c(param0){
    modify_field(pkt.field_f_16, param0);
}

action action_d(){
    no_op();
}

action action_e(){
    no_op();
}

table table_a {
    reads {
         pkt.field_a_32  : exact;
    }
    actions {
        action_a;
    }
    size : 256;
}

table table_b {
    reads {
         pkt.field_a_32  : exact;
    }
    actions {
        action_b;
    }
    size : 256;
}

table table_c {
    reads {
         pkt.field_a_32  : exact;
    }
    actions {
        action_c;
    }
    size : 256;
}

@pragma stage 3
table table_d {
    reads {
         pkt.field_e_16  : exact;
    }
    actions {
        action_d;
    }
    size : 256;
}

@pragma stage 4
table table_e {
    reads {
         pkt.field_g_16  : exact;
    }
    actions {
        action_e;
    }
    size : 256;
}


/* Main control flow */

control ingress {
        apply(table_a);
        apply(table_b);
        apply(table_c);
        apply(table_d);
        apply(table_e);
}


