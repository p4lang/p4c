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
       n_4  : 4;
       o_1  : 1;
       p_3  : 3;
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

action set_flag(){
   modify_field(my_test_config_1.o_1, 1);
}

action do_nothing(){
   no_op();
}

table test_exact_table {
   reads {
      //my_test_config_1.a_32 : exact;
      //my_test_config_1.a_32 mask 0xff0fff0f: exact;
      //my_test_config_1.p_3 : valid;

      my_test_config_1.a_32 : exact;
      my_test_config_1.e_32 : exact;
      my_test_config_1.f_32 : exact;
      my_test_config_1.g_32 : exact;
      my_test_config_1.h_32 : exact;
      my_test_config_1.i_32 : exact;


   }

   actions {
      set_flag;
      do_nothing;
   }
   max_size : 16384;
}

control ingress {
   apply(test_exact_table);
}
