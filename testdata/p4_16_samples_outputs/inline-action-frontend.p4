control p(inout bit<1> bt) {
    @name("a") action a_0(inout bit<1> y0_0) {
        y0_0 = y0_0 | 1w1;
    }
    @name("b") action b_0() {
        a_0(bt);
        a_0(bt);
    }
    @name("t") table t_0 {
        actions = {
            b_0();
        }
        default_action = b_0();
    }
    apply {
        t_0.apply();
    }
}

control simple<T>(inout T arg);
package m<T>(simple<T> pipe);
m<bit<1>>(p()) main;

