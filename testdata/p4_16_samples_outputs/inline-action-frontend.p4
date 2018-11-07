control p(inout bit<1> bt) {
    @name("p.b") action b() {
        {
            bit<1> y0 = bt;
            y0 = y0 | 1w1;
            bt = y0;
        }
        {
            bit<1> y0_1 = bt;
            y0_1 = y0_1 | 1w1;
            bt = y0_1;
        }
    }
    @name("p.t") table t_0 {
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

