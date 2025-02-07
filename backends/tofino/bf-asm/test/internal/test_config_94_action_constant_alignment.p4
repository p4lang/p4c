#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {

       field_x_12  : 12;
       field_x_4  : 4;
       field_x_16   : 16;

       field_a_32 : 32;
       field_b_32 : 32;
       field_c_32 : 32;


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

header_type tags_t {
    fields {
       field_a_8 : 8;
       field_b_12 : 12;
       field_c_4 : 4;
       field_d_8 : 8;
    }
}



header pkt_t pkt;
header tags_t tags[5];

parser start {
    return parse_test;
}

parser parse_test {
    extract(pkt);
    extract(tags[0]);
    return ingress;
}


action nop(){
   no_op();
}

action action_0(){
   modify_field(pkt.field_x_12, 0xfff);
   modify_field(pkt.field_x_4, 0);
}


@pragma immediate 0
table table_0 {
   reads {
      pkt.field_a_32 : ternary;
   }
   actions {
      action_0;
   }
   size : 1024;
}

control ingress {
    apply(table_0);
}
