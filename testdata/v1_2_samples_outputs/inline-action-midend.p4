control p(inout bit<1> bt) {
    @name("b") action b_0() {
        bt = bt | 1w1;
        bt = bt | 1w1;
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
