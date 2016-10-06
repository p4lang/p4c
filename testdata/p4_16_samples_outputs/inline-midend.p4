control p() {
    @name("x") bit<1> x_2;
    @name("z") bit<1> z;
    @name("x") bit<1> x_3;
    @name("y") bit<1> y_1;
    @name("tmp_0") bit<1> tmp_7;
    @name("tmp_1") bit<1> tmp_8;
    @name("tmp_2") bit<1> tmp_9;
    @name("tmp_3") bit<1> tmp_10;
    @name("tmp_4") bit<1> tmp_11;
    @name("x0_0") bit<1> x0;
    @name("y0_0") bit<1> y0;
    @name("tmp") bit<1> tmp_12;
    @name("x0_1") bit<1> x0_2;
    @name("y0_1") bit<1> y0_2;
    @name("tmp_5") bit<1> tmp_13;
    @name("tmp_6") bit<1> tmp_14;
    @name("x") bit<1> x_0;
    @name("y") bit<1> y_0;
    @name("b") action b_0() {
        x_0 = tmp_13;
        tmp_7 = x_0;
        @name("a") {
            x0 = tmp_7;
            x_2 = x0;
            tmp_12 = x0 & x_2;
            y0 = tmp_12;
            tmp_8 = y0;
        }
        z = tmp_8;
        tmp_9 = z & z;
        tmp_10 = tmp_9;
        @name("a") {
            x0_2 = tmp_10;
            x_2 = x0_2;
            tmp_12 = x0_2 & x_2;
            y0_2 = tmp_12;
            tmp_11 = y0_2;
        }
        y_0 = tmp_11;
        tmp_14 = y_0;
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
