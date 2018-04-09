extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp_7;
    bool tmp_9;
    bit<32> tmp_10;
    bool tmp_12;
    bit<32> tmp_13;
    @hidden action act() {
        tmp_9 = false;
    }
    @hidden action act_0() {
        tmp_10 = f(32w3);
        tmp_9 = tmp_10 < 32w0;
    }
    @hidden action act_1() {
        tmp_7 = f(32w2);
    }
    @hidden action act_2() {
        tmp_12 = true;
    }
    @hidden action act_3() {
        tmp_13 = f(32w5);
        tmp_12 = tmp_13 == 32w2;
    }
    @hidden action act_4() {
        r = 32w1;
    }
    @hidden action act_5() {
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
        tbl_act.apply();
        if (!(tmp_7 > 32w0)) 
            tbl_act_0.apply();
        else {
            tbl_act_1.apply();
        }
        if (tmp_9) 
            tbl_act_2.apply();
        else {
            tbl_act_3.apply();
        }
        if (tmp_12) 
            tbl_act_4.apply();
        else 
            tbl_act_5.apply();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

