control p(inout bit<1> bt) {
    @name("b") action b() {
        bt = bt | 1w1;
        bt = bt | 1w1;
    }
    @name("t") table t_0() {
        actions = {
            b;
        }
        default_action = b;
    }
    apply {
        t_0.apply();
    }
}

package m(p pipe);
m(p()) main;
