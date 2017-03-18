control p(out bit<1> y) {
    bit<1> x_2;
    bit<1> z;
    bit<1> x_3;
    bit<1> tmp_5;
    bit<1> tmp_6;
    bit<1> tmp_7;
    bit<1> tmp_8;
    bit<1> tmp_9;
    bit<1> tmp_10;
    bit<1> x0;
    bit<1> y0;
    bit<1> x0_2;
    bit<1> y0_2;
    bit<1> x_0;
    bit<1> y_0;
    @name("b") action b_0() {
        x_0 = tmp_9;
        tmp_5 = tmp_9;
        x0 = tmp_9;
        x_2 = tmp_9;
        y0 = tmp_9 & tmp_9;
        tmp_6 = tmp_9 & tmp_9;
        z = tmp_9 & tmp_9;
        tmp_7 = tmp_9 & tmp_9 & (tmp_9 & tmp_9);
        x0_2 = tmp_9 & tmp_9 & (tmp_9 & tmp_9);
        x_2 = tmp_9 & tmp_9 & (tmp_9 & tmp_9);
        y0_2 = tmp_9 & tmp_9 & (tmp_9 & tmp_9) & (tmp_9 & tmp_9 & (tmp_9 & tmp_9));
        tmp_8 = tmp_9 & tmp_9 & (tmp_9 & tmp_9) & (tmp_9 & tmp_9 & (tmp_9 & tmp_9));
        y_0 = tmp_9 & tmp_9 & (tmp_9 & tmp_9) & (tmp_9 & tmp_9 & (tmp_9 & tmp_9));
        tmp_10 = tmp_9 & tmp_9 & (tmp_9 & tmp_9) & (tmp_9 & tmp_9 & (tmp_9 & tmp_9));
    }
    action act() {
        x_3 = 1w1;
        tmp_9 = 1w1;
    }
    action act_0() {
        y = tmp_10;
    }
    table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_b {
        actions = {
            b_0();
        }
        const default_action = b_0();
    }
    table tbl_act_0 {
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
