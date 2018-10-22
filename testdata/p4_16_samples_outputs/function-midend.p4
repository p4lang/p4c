control c(out bit<16> b) {
    bool hasReturned;
    bit<16> retval;
    @hidden action act() {
        hasReturned = true;
        retval = 16w12;
    }
    @hidden action act_0() {
        hasReturned = false;
    }
    @hidden action act_1() {
        b = retval;
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        tbl_act.apply();
        if (!hasReturned) {
            tbl_act_0.apply();
        }
        tbl_act_1.apply();
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;

