control p() {
    bit<1> x_2;
    bit<1> z;
    bit<1> x_3;
    bit<1> y_1;
    bit<1> tmp_7;
    bit<1> tmp_8;
    bit<1> tmp_9;
    bit<1> tmp_10;
    bit<1> tmp_11;
    bit<1> tmp_12;
    bit<1> tmp_13;
    bit<1> tmp_14;
    bit<1> x0;
    bit<1> y0;
    bit<1> x0_2;
    bit<1> y0_2;
    @name("x") bit<1> x_0;
    @name("y") bit<1> y_0;
    @name("b") action b_0() {
        x_0 = tmp_13;
        tmp_8 = tmp_13;
        x0 = tmp_13;
        x_2 = tmp_13;
        tmp_7 = tmp_13 & tmp_13;
        y0 = tmp_13 & tmp_13;
        tmp_9 = tmp_13 & tmp_13;
        z = tmp_13 & tmp_13;
        tmp_10 = tmp_13 & tmp_13 & (tmp_13 & tmp_13);
        tmp_11 = tmp_13 & tmp_13 & (tmp_13 & tmp_13);
        x0_2 = tmp_13 & tmp_13 & (tmp_13 & tmp_13);
        x_2 = tmp_13 & tmp_13 & (tmp_13 & tmp_13);
        tmp_7 = tmp_13 & tmp_13 & (tmp_13 & tmp_13) & (tmp_13 & tmp_13 & (tmp_13 & tmp_13));
        y0_2 = tmp_13 & tmp_13 & (tmp_13 & tmp_13) & (tmp_13 & tmp_13 & (tmp_13 & tmp_13));
        tmp_12 = tmp_13 & tmp_13 & (tmp_13 & tmp_13) & (tmp_13 & tmp_13 & (tmp_13 & tmp_13));
        y_0 = tmp_13 & tmp_13 & (tmp_13 & tmp_13) & (tmp_13 & tmp_13 & (tmp_13 & tmp_13));
        tmp_14 = tmp_13 & tmp_13 & (tmp_13 & tmp_13) & (tmp_13 & tmp_13 & (tmp_13 & tmp_13));
    }
    action act() {
        x_3 = 1w1;
        tmp_13 = x_3;
    }
    action act_0() {
        y_1 = tmp_14;
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

control simple();
package m(simple pipe);
m(p()) main;
