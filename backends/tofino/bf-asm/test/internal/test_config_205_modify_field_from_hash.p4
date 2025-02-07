
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

field_list simple_fields {
    pkt.field_e_16;
    pkt.field_f_16;
}


field_list_calculation simple_hash {
    input { simple_fields; }
    algorithm : identity_lsb;
    output_width : 31;
}

field_list_calculation simple_hash_2 {
    input { simple_fields; }
    algorithm : identity_msb;
    output_width : 31;
}


action common_action(){
    modify_field_with_hash_based_offset(pkt.field_b_32, 0, simple_hash, 2147483648);
}

action action_0(){
    common_action();
    modify_field(pkt.field_k_8, 0);
}

action action_1(){
    common_action();
    modify_field(pkt.field_k_8, 1);
}

action action_2(){
    common_action();
    modify_field(pkt.field_k_8, 2);
    //modify_field_with_hash_based_offset(pkt.field_c_32, 0, simple_hash_2, 2147483648);
}

table table_0 {
    reads {
        pkt.field_a_32 : lpm;
    }
    
    actions {
        action_0;
        action_1;
        action_2;
    }
    size : 512;
}


control ingress {
    apply(table_0);
}

