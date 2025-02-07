#include "tofino/intrinsic_metadata.p4"


/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_32 : 32;
        field_b_32 : 32 (signed);
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

        field_x_32 : 32 (signed);
    }
}

header_type to_add_t {
     fields {
          field_one : 16;
          field_two : 8;
          field_three : 32;
          field_four : 8;
     }
}

parser start {
    return parse_ethernet;
}

header pkt_t pkt;
header to_add_t to_add;

parser parse_ethernet {
    extract(pkt);
    return select(pkt.field_i_8){
        1 : parse_to_add;
        default: ingress;
    }
}

parser parse_to_add {
    extract(to_add);
    return ingress;
}

action action_0(){
    add_header(to_add);
    modify_field(to_add.field_two, 15);
}

action do_nothing(){}

table table_0 {
    reads {
        pkt.field_a_32 : ternary;
        to_add.field_one : exact;
        to_add.field_three : exact;
        to_add.field_four : exact;
    }
    actions {
        action_0;
        do_nothing;
    }
    size : 512;
}

/* Main control flow */

control ingress {
    apply(table_0);
}
