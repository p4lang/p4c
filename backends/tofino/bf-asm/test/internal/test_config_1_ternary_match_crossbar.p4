#include "tofino/intrinsic_metadata.p4"

header_type my_test_config_1_t {
   fields {
       a_32 : 32;
       b_8 : 8;
       c_8 : 8;
       d_16 : 16;
       e_32 : 32;
   }
}


header my_test_config_1_t my_test_config_1;

parser start{
   return parse_my_test_config_1;
}

parser parse_my_test_config_1{
   extract(my_test_config_1);
   return ingress;
}

action modify_b(my_param){
   modify_field(my_test_config_1.b_8, my_param);
}

action just_no_op(){
   no_op();
}


table my_test_config_1_table {
   reads {
      my_test_config_1.a_32 : lpm;
      my_test_config_1.e_32 : ternary;
      //my_test_config_1.d_16 : ternary;
      my_test_config_1.b_8 mask 0xf0: ternary;
      my_test_config_1.c_8 : ternary;
   }
   actions {
      modify_b;
      just_no_op;
   }
   max_size : 2048;
}

control ingress {
   apply(my_test_config_1_table);
}
