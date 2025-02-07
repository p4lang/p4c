
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

parser start {
    return parse_ethernet;
}

header pkt_t pkt;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

action action_0(){
    bit_nor(pkt.field_a_32, pkt.field_b_32, pkt.field_c_32);
}
action action_1(param0){
     bit_orca(pkt.field_b_32, param0, pkt.field_c_32);
}

action do_nothing(){}

table table_0 {
    actions {
        action_0;
    }
}

table table_1 {
    actions {
        action_1;
    }
}

table table_2 {
    actions {
        do_nothing;
    }
}

/* Main control flow */

control ingress {
    apply(table_0);
    apply(table_1);
    if (pkt.field_i_8 == 0){
       apply(table_2);
    }
}
