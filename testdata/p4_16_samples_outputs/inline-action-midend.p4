control p(inout bit<1> bt) {
    @name("p.b") action b() {
        bt = bt | 1w1;
        bt = bt | 1w1;
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

