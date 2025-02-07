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
        color_0 : 4;
        pad_0 : 4;
        color_1 : 8;
        color_2 : 8;
        color_3 : 8;
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


action action_0(param0){
    modify_field(pkt.field_f_16, param0);
}


action action_1(param0){
    modify_field(pkt.field_g_16, param0);
}


action do_nothing(){
    no_op();
}

table table_0 {
    reads {
        pkt.field_e_16 : ternary;
    }
    actions {
        action_0;
        do_nothing;
    }
    size : 1024;
}

//@pragma no_versioning 1
table table_1 {
    reads {
        pkt.field_e_16: exact;
        pkt.field_f_16 mask 0xfffc : exact;
    }
    actions {
       action_1;
       do_nothing;

    }
    size : 1024;
}

table table_2 {
    reads {
        pkt.field_i_8: ternary;
    }
    actions {
       do_nothing;

    }
    size : 256;
}


/* Main control flow */

control ingress {
      if (valid(pkt)){
          apply(table_0);
      } else {
          apply(table_1) {
              action_1 {
                  apply(table_2);
              }
          }
      }
}
