control p(inout bit<1> bt) {
    @name("a") action a_0(inout bit<1> y0) {
        bit<1> tmp;
        tmp = y0 | 1w1;
        y0 = tmp;
    }
    @name("b") action b_0() {
        a_0(bt);
        a_0(bt);
    }
    @name("t") table t_0() {
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
