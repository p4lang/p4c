control c(inout bit<32> x) {
    bit<32> tmp_2;
    @hidden action act() {
        tmp_2 = x;
    }
    @hidden action act_0() {
        tmp_2 = x;
    }
    @hidden action act_1() {
        x = x + tmp_2;
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
    apply {
        if (x > x) 
            tbl_act.apply();
        else 
            tbl_act_0.apply();
        tbl_act_1.apply();
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

