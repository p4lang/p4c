extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("tmp") bit<32> tmp_1;
    @name("tmp_0") bool tmp_2;
    action act() {
        r = 32w1;
    }
    action act_0() {
        r = 32w2;
    }
    action act_1() {
        tmp_1 = f(32w2);
        tmp_2 = tmp_1 > 32w0;
    }
    table tbl_act() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (tmp_2) 
            tbl_act_0.apply();
        else 
            tbl_act_1.apply();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
