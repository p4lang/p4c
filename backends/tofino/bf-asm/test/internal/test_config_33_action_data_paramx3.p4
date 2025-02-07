#include "tofino/intrinsic_metadata.p4"

header_type test_t {
   fields {
      field_a : 4;
      field_b : 8;
      field_c : 4;
      field_d : 32;
      field_e : 32;
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


action action_0(my_param_0, my_param_1, my_param_2){
   modify_field(test.field_a, my_param_0);
   modify_field(test.field_b, my_param_1);
   modify_field(test.field_c, my_param_2);
}


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
