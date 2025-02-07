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
        pad_1 : 16;
        pad_2 : 24;
        color_1 : 8;
        pad_3 : 24;
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


meter meter_0 {
    type : bytes;
    //static : table_0;
    direct : table_0;
    result : pkt.color_0;
    //instance_count : 500;
}


meter meter_1 {
    type : bytes;
    static : table_1;
    //direct : table_1;
    result : pkt.color_1;
    instance_count : 500;
}


action action_0(param0){
    //modify_field(pkt.field_f_16, param0);
    //execute_meter(meter_0, 7, pkt.color_0);
}


action action_1(param0){
    //modify_field(pkt.field_g_16, param0);
    execute_meter(meter_1, 7, pkt.color_1);
}


action do_nothing(){
    no_op();
//    drop();
}



//@pragma include_idletime 1
@pragma pa_solitare meter_result.color_0, meter_result.color_1
//@pragma include_stash 1
table table_0 {
    reads {
        pkt.field_e_16 : ternary;
        pkt.color_0 : exact;  //HACK
        pkt.color_1 : exact;  //HACK
    }
    actions {
        action_0;
//        do_nothing;
    }
    size : 6000;
}


//@pragma include_idletime 1
@pragma idletime_two_way_notification 1
//@pragma include_stash 1
table table_1 {
    reads {
        pkt.field_e_16: exact;
    }
    actions {
       do_nothing;
       action_1;
    }
    size : 32768;
}


/* Main control flow */

control ingress {
      apply(table_0);
      apply(table_1);
}
