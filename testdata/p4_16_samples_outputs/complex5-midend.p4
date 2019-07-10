extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    @hidden action act() {
        r = 32w1;
    }
    @hidden action act_0() {
        r = 32w2;
    }
    @hidden action act_1() {
        tmp = f(32w2);
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
        if (tmp > 32w0) {
            tbl_act_0.apply();
        } else {
            tbl_act_1.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

