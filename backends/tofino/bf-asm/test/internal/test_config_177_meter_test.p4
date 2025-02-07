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
        color_0 : 8;
        pad_0 : 24;
        pad_1 : 160;
        pad_2 : 24;
        color_1 : 8;
        pad_3 : 24;

        lpf : 16;
    }
}

header_type meta_t {
    fields {
        color_0 : 8;
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


meter meter_0 {
    type : bytes;
    direct : table_0;
    result : pkt.color_0;
    //static : table_0;
    //instance_count : 500;
}

action action_0(param0){
    //execute_meter(meter_0, 7, pkt.color_0);
}

action action_1(param0){
    //execute_meter(meter_0, 0, pkt.color_1);
}

action do_nothing(){
    no_op();
}


table table_0 {
    reads {
        pkt.field_e_16 : ternary;
        pkt.color_0 : exact;  /* HACK */
        pkt.color_1 : exact;  /* HACK */
        //meta.color_0 : exact;
    }
    actions {
        action_0;
        action_1;
        do_nothing;
    }
    size : 6000;
}



/* Main control flow */

control ingress {
      apply(table_0);
}
