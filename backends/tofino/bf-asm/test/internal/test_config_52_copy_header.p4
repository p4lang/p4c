#include "tofino/intrinsic_metadata.p4"

header_type test_t {
   fields {
      field_a : 8;
      field_b : 16;
      field_c : 8;
      field_d : 32;
      field_e : 16;
      field_f : 16;
   }
}


header test_t test_dest;
header test_t test_src;

parser start {
    return parse_test;
}

parser parse_test {
    extract(test_dest);
    extract(test_src);
    return ingress;
}


action action_0(){
   copy_header(test_dest, test_src);
}


table table_0 {
   reads {
      test_src.field_e : ternary;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
