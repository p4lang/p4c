extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bit<32> tmp_1;
    @hidden action act() {
        r = 32w1;
    }
    @hidden action act_0() {
        r = 32w3;
    }
    @hidden action act_1() {
        tmp = f(32w2);
    }
    @hidden action act_2() {
        r = 32w2;
    }
    @hidden action act_3() {
        tmp_1 = f(32w2);
    }
    @hidden table tbl_act {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_1 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        if (tmp_1 > 32w0) {
            tbl_act_0.apply();
            if (tmp < 32w2) {
                tbl_act_1.apply();
            } else {
                tbl_act_2.apply();
            }
        } else {
            tbl_act_3.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

