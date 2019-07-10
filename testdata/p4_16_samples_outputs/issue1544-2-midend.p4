control c(inout bit<32> x) {
    bit<32> tmp_4;
    bit<32> tmp_11;
    bit<32> tmp_12;
    @hidden action act() {
        tmp_4 = x + 32w1;
    }
    @hidden action act_0() {
        tmp_4 = x;
    }
    @hidden action act_1() {
        tmp_11 = x + 32w4294967295;
    }
    @hidden action act_2() {
        tmp_11 = x;
    }
    @hidden action act_3() {
        tmp_12 = tmp_11;
    }
    @hidden action act_4() {
        tmp_12 = tmp_4;
    }
    @hidden action act_5() {
        x = tmp_12;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
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
    @hidden table tbl_act_4 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    @hidden table tbl_act_5 {
        actions = {
            act_5();
        }
        const default_action = act_5();
    }
    apply {
        if (x > x + 32w1) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        if (x > x + 32w4294967295) {
            tbl_act_1.apply();
        } else {
            tbl_act_2.apply();
        }
        if (tmp_4 > tmp_11) {
            tbl_act_3.apply();
        } else {
            tbl_act_4.apply();
        }
        tbl_act_5.apply();
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

