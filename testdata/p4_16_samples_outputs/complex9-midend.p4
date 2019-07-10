extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bool tmp_1;
    bit<32> tmp_2;
    @hidden action act() {
        tmp_1 = false;
    }
    @hidden action act_0() {
        tmp_2 = f(32w3);
        tmp_1 = tmp_2 < 32w0;
    }
    @hidden action act_1() {
        tmp = f(32w2);
    }
    @hidden action act_2() {
        r = 32w1;
    }
    @hidden action act_3() {
        r = 32w2;
    }
    @hidden table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    apply {
        tbl_act.apply();
        if (!(tmp > 32w0)) {
            tbl_act_0.apply();
        } else {
            tbl_act_1.apply();
        }
        if (tmp_1) {
            tbl_act_2.apply();
        } else {
            tbl_act_3.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

