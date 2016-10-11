control c(inout bit<32> x) {
    bit<32> tmp_3;
    bit<32> tmp_4;
    bit<32> tmp_5;
    bool tmp_6;
    action act() {
        tmp_3 = x + 32w2;
        x = tmp_3;
        tmp_4 = x + 32w4294967290;
        x = tmp_4;
    }
    action act_0() {
        tmp_5 = x << 2;
        x = tmp_5;
    }
    action act_1() {
        x = 32w10;
        tmp_6 = x == 32w10;
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
        if (tmp_6) {
            tbl_act_0.apply();
        }
        else {
            tbl_act_1.apply();
        }
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;
