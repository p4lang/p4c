control p(inout bit<1> bt) {
    bit<1> y0_0;
    bit<1> y0_1;
    @name("b") action b() {
        y0_0 = bt;
        y0_0 = y0_0 | 1w1;
        bt = y0_0;
        y0_1 = bt;
        y0_1 = y0_1 | 1w1;
        bt = y0_1;
    }
    @name("t") table t_0() {
        actions = {
            b();
        }
        default_action = b();
    }
    apply {
        t_0.apply();
    }
}

control simple<T>(inout T arg);
package m<T>(simple<T> pipe);
m<bit<1>>(p()) main;
