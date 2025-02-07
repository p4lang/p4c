#include "tofino/intrinsic_metadata.p4"

header_type test_t {
   fields {
      field_0 : 1;
      field_1 : 1;
      field_2 : 1;
      field_3 : 1;
      field_4 : 1;
      field_5 : 1;
      field_6 : 1;
      field_7 : 1;
      field_8 : 1;
      field_9 : 1;
      field_10 : 1;
      field_11 : 1;
      field_12 : 1;
      field_13 : 1;
      field_14 : 1;
      field_15 : 1;
      field_16 : 1;
      field_17 : 1;
      field_18 : 1;
      field_19 : 1;
      field_20 : 1;
      field_21 : 1;
      field_22 : 1;
      field_23 : 1;
      field_24 : 1;
      field_25 : 1;
      field_26 : 1;
      field_27 : 1;
      field_28 : 1;
      field_29 : 1;
      field_30 : 1;
      field_31 : 1;

      field_a : 32;
      field_b : 32;
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


action action_0(my_param_0, my_param_1, my_param_2,
                my_param_4, my_param_5,
                my_param_6, my_param_7, my_param_8,
                my_param_10, my_param_11,
                my_param_12, my_param_13, my_param_14,
                my_param_16, my_param_17,
                my_param_18, my_param_19, my_param_20,
                my_param_21, my_param_22, my_param_23,
                my_param_25, my_param_26,
                my_param_27, my_param_28,
                my_param_31){

   modify_field(test.field_0, my_param_0);
   modify_field(test.field_1, my_param_1);
   modify_field(test.field_2, my_param_2);
   modify_field(test.field_3, 0);
   modify_field(test.field_4, my_param_4);
   modify_field(test.field_5, my_param_5);
   modify_field(test.field_6, my_param_6);
   modify_field(test.field_7, my_param_7);
   modify_field(test.field_8, my_param_8);
   modify_field(test.field_9, 1);
   modify_field(test.field_10, my_param_10);
   modify_field(test.field_11, my_param_11);
   modify_field(test.field_12, my_param_12);
   modify_field(test.field_13, my_param_13);
   modify_field(test.field_14, my_param_14);
   modify_field(test.field_15, 1);
   modify_field(test.field_16, my_param_16);
   modify_field(test.field_17, my_param_17);
   modify_field(test.field_18, my_param_18);
   modify_field(test.field_19, my_param_19);
   modify_field(test.field_20, my_param_20);
   modify_field(test.field_21, my_param_21);
   modify_field(test.field_22, my_param_22);
   modify_field(test.field_23, my_param_23);
   modify_field(test.field_24, 0);
   modify_field(test.field_25, my_param_25);
   modify_field(test.field_26, my_param_26);
   modify_field(test.field_27, my_param_27);
   modify_field(test.field_28, my_param_28);
   modify_field(test.field_29, 1);
   modify_field(test.field_30, 1);
   modify_field(test.field_31, my_param_31);
}

table table_0 {
   reads {
      test.field_a : ternary;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
