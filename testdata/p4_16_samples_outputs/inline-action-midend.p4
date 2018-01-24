control p(inout bit<1> bt) {
    @name("p.b") action b_0() {
        bt = bt | 1w1;
        bt = bt | 1w1;
    }
    @name("p.t") table t {
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

