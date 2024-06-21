control p(inout bit<1> bt) {
    @name("p.y0") bit<1> y0_0_inlined_a;
    @name("p.y0") bit<1> y0_0_inlined_a_0;
    @name("p.b") action b() {
        y0_0_inlined_a = bt;
        y0_0_inlined_a = y0_0_inlined_a | 1w1;
        bt = y0_0_inlined_a;
        y0_0_inlined_a_0 = bt;
        y0_0_inlined_a_0 = y0_0_inlined_a_0 | 1w1;
        bt = y0_0_inlined_a_0;
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
