
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


action action_0(){
    modify_field(pkt.field_k_8, 0);
}

action action_1(){
    modify_field(pkt.field_k_8, 1);
}

table table_0 {
    reads {
        pkt.field_a_32 : lpm;
    }
    
    actions {
        action_0;
        action_1;
    }
    size : 512;
}


control ingress {
    if (not valid(pkt)){
        apply(table_0);
    }
}

