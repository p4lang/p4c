#include "tofino/intrinsic_metadata.p4"

header_type test_t {
   fields {
      field_a : 32;
      field_b : 32;
      field_c : 32;
      field_d : 32;
      field_e : 16;
      field_f : 16;
      field_g : 16;
      field_h : 16;
      field_i : 2;
      field_j : 2;
      field_k : 2;
      field_l : 2;
   }
}


header test_t test;

parser start {
    return parse_test;
}

parser parse_test {
    extract(test);
    return ingress;
}


action action_0(my_param_0, my_param_1){
   modify_field(test.field_i, my_param_0);
   modify_field(test.field_k, my_param_1);
}

@pragma immediate 1
table table_0 {
   reads {
      test.field_e : ternary;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
