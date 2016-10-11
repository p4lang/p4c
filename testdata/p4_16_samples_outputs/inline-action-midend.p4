control p(inout bit<1> bt) {
    bit<1> tmp_0;
    bit<1> y0;
    bit<1> y0_2;
    @name("b") action b_0() {
        y0 = bt;
        tmp_0 = bt | 1w1;
        y0 = bt | 1w1;
        bt = bt | 1w1;
        y0_2 = bt | 1w1;
        tmp_0 = bt | 1w1 | 1w1;
        y0_2 = bt | 1w1 | 1w1;
        bt = bt | 1w1 | 1w1;
    }
    @name("t") table t() {
        actions = {
            b_0();
        }
        default_action = b_0();
    }
    apply {
        t.apply();
    }
}

control simple<T>(inout T arg);
package m<T>(simple<T> pipe);
m<bit<1>>(p()) main;
