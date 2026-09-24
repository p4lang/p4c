extern void __e(in bit<1> x=1w1);
control C() {
    @hidden action issue5473_param_bit_type_default_value5() {
        __e(x = 1w1);
        __e(x = 1w1);
    }
    @hidden table tbl_issue5473_param_bit_type_default_value5 {
        actions = {
            issue5473_param_bit_type_default_value5();
        }
        const default_action = issue5473_param_bit_type_default_value5();
    }
    apply {
        tbl_issue5473_param_bit_type_default_value5.apply();
    }
}

control proto();
package top(proto p);
top(C()) main;
