extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("tmp") bit<32> tmp_7;
    @name("tmp_0") bool tmp_8;
    @name("tmp_1") bool tmp_9;
    @name("tmp_2") bit<32> tmp_10;
    @name("tmp_3") bool tmp_11;
    @name("tmp_4") bool tmp_12;
    @name("tmp_5") bit<32> tmp_13;
    @name("tmp_6") bool tmp_14;
    action act() {
        tmp_9 = false;
    }
    action act_0() {
        tmp_10 = f(32w3);
        tmp_11 = tmp_10 < 32w0;
        tmp_9 = tmp_11;
    }
    action act_1() {
        tmp_7 = f(32w2);
        tmp_8 = tmp_7 > 32w0;
    }
    action act_2() {
        tmp_12 = true;
    }
    action act_3() {
        tmp_13 = f(32w5);
        tmp_14 = tmp_13 == 32w2;
        tmp_12 = tmp_14;
    }
    action act_4() {
        r = 32w1;
    }
    action act_5() {
        r = 32w2;
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
    table tbl_act_2() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_3() {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    table tbl_act_4() {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    table tbl_act_5() {
        actions = {
            act_5();
        }
        const default_action = act_5();
    }
    apply {
        tbl_act.apply();
        if (!tmp_8) 
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
