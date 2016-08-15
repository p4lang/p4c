extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bit<32> tmp_0;
    action act() {
        r = 32w1;
    }
    action act_0() {
        r = 32w3;
    }
    action act_1() {
        tmp = f(32w2);
    }
    action act_2() {
        r = 32w2;
    }
    action act_3() {
        tmp_0 = f(32w2);
    }
    table tbl_act() {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    table tbl_act_0() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_1() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_2() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_3() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        if (tmp_0 > 32w0) {
            tbl_act_0.apply();
            if (tmp < 32w2) 
                tbl_act_1.apply();
            else 
                tbl_act_2.apply();
        }
        else 
            tbl_act_3.apply();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
