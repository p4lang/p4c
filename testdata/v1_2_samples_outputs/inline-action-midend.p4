control p(inout bit<1> bt) {
    bit<1> y0_0;
    bit<1> y0_1;
    @name("b") action b_0() {
        y0_0 = bt;
        y0_0 = y0_0 | 1w1;
        bt = y0_0;
        y0_1 = bt;
        y0_1 = y0_1 | 1w1;
        bt = y0_1;
    }
    @name("t") table t_0() {
        actions = {
            b_0;
        }
        default_action = b_0;
    }
    apply {
        t_0.apply();
    }
}

package m(p pipe);
m(p()) main;
