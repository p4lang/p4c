
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

header_type meta_t {
     fields {
         hash_1 : 16;
     }
}

parser start {
    return parse_ethernet;
}

header pkt_t pkt;
metadata meta_t meta;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

field_list fields_for_hash {
    pkt.field_c_32;
    pkt.field_d_32;
}

field_list_calculation hash_1 {
    input { fields_for_hash; }
    algorithm: crc16;
    output_width: 16;
}


action action_0(){
    //modify_field_with_hash_based_offset(meta.hash_1, 0, hash_1, 65536);  //16
    modify_field_with_hash_based_offset(meta.hash_1, 0, hash_1, 16777216);  //24
    //modify_field_with_hash_based_offset(meta.hash_1, 0, hash_1, 4294967296);  //32
}

action do_nothing(){}

table table_0 {
    reads {
        pkt.field_g_16 : ternary;
    }
    actions {
        action_0;
        do_nothing;
    }
}


/* Main control flow */

control ingress {
    apply(table_0);
}
