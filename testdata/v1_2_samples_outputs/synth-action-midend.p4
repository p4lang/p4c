control c(inout bit<32> x) {
    action act() {
        x = x + 32w2;
        x = x + 32w4294967290;
    }
    action act_0() {
        x = x << 2;
    }
    action act_1() {
        x = 32w10;
    }
    table tbl_act() {
        actions = {
            act_1;
        }
        const default_action = act_1();
    }
    table tbl_act_0() {
        actions = {
            act;
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (x == 32w10) {
            tbl_act_0.apply();
        }
        else 
            tbl_act_1.apply();
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;
