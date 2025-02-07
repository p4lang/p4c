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

        field_o_10 : 10;
        field_p_6  : 6;
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

action action_0(){
    modify_field(pkt.field_f_16, 1);
}

action do_nothing(){
    no_op();
}

action do_nothing_1(){ 
}

table table_z { 
    reads {
        pkt.field_l_8 : ternary;
    }
    actions { 
        do_nothing;
    }
    size : 512;
}


table table_a { 
    reads {
        pkt.field_a_32 : ternary;
    }
    actions { 
        do_nothing;
    }
    size : 512;
}

table table_b { 
    reads {
        pkt.field_b_32 : ternary;
    }
    actions { 
        do_nothing;
    }
    size : 512;
}

table table_c { 
    reads {
        pkt.field_c_32 : ternary;
    }
    actions { 
        do_nothing;
    }
    size : 512;
}

table table_d { 
    reads {
        pkt.field_d_32 : ternary;
    }
    actions { 
        do_nothing;
    }
    size : 512;
}


@pragma entries_with_ranges 64
@pragma tcam_error_detect 1
table table_e {
    reads {
         pkt.field_o_10 : range;
         pkt.field_g_16 : exact;
         pkt.field_h_16 : exact;
    }
    actions {
        do_nothing;
        action_0;
    }
    size : 1024;
}

@pragma tcam_error_detect 1
table table_f {
    reads {
         pkt.field_g_16 : ternary;
    }
    actions {
        do_nothing;
    }
    size : 1024;
}

/* Main control flow */

// Should fit in two stages
control ingress {
    apply(table_z);
    apply(table_a);
    apply(table_b);
    apply(table_c);
    apply(table_d);
    apply(table_e);
    apply(table_f);
}






