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


action action_0(){
   modify_field(test.field_a, test.field_e);
}

table table_0 {
   reads {
      test.field_b : ternary;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
