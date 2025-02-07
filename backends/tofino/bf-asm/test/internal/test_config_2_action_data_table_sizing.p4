#include "tofino/intrinsic_metadata.p4"


header_type my_test_config_1_t {
   fields {
       a_32 : 32;
       b_8 : 8;
       c_8 : 8;
       d_16 : 16;
       e_32 : 32;
       f_32 : 32;
       g_32 : 32;
       h_32 : 32;
       i_32 : 32;
       j_16 : 16;
       k_16 : 16;
       l_16 : 16;
       m_16 : 16;
   }
}


header my_test_config_1_t my_test_config_1;

parser start{
   return parse_my_test_config_1;
}

parser parse_my_test_config_1 {
   extract(my_test_config_1);
   return ingress;
}

action action_160(param_1_32, param_2_32, param_3_32, param_4_32, param_5_16, param_6_8){
   modify_field(my_test_config_1.a_32, param_1_32);
   modify_field(my_test_config_1.e_32, param_2_32);
   modify_field(my_test_config_1.f_32, param_3_32);
   //modify_field(my_test_config_1.g_32, param_4_32);
   modify_field(my_test_config_1.m_16, param_5_16);
   modify_field(my_test_config_1.c_8, param_6_8);
}

action action_8(param_1_8){
   modify_field(my_test_config_1.b_8, param_1_8);
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
      action_160;
      action_8;
   }
   max_size : 1024;
}

control ingress {
   apply(my_test_config_1_table);
}
