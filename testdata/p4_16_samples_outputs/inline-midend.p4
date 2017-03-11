control p(out bit<1> y) {
    bit<1> tmp_13;
    bit<1> tmp_14;
    @name("b") action b_0() {
        tmp_14 = tmp_13 & tmp_13 & (tmp_13 & tmp_13) & (tmp_13 & tmp_13 & (tmp_13 & tmp_13));
    }
    action act() {
        tmp_13 = 1w1;
    }
    action act_0() {
        y = tmp_14;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_b() {
        actions = {
            b_0();
        }
        const default_action = b_0();
    }
    table tbl_act_0() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        tbl_b.apply();
        tbl_act_0.apply();
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;
