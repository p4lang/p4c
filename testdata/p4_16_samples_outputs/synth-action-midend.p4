control c(inout bit<32> x) {
    @hidden action act() {
        x = 32w12;
        x = 32w6;
    }
    @hidden action act_0() {
        x = 32w40;
    }
    @hidden action act_1() {
        x = 32w10;
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
    apply {
        tbl_act.apply();
        if (true) {
            tbl_act_0.apply();
        }
        else 
            tbl_act_1.apply();
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;
