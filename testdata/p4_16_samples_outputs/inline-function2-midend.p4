control p(inout bit<1> bt) {
    @name("p.tmp") bit<1> tmp;
    @name("p.y0") bit<1> y0;
    @name("p.b") action b() {
        y0 = bt;
        if (bt + 1w1 > 1w0) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        if (tmp == 1w1) {
            y0 = 1w0;
        } else if (bt + 1w1 != 1w1) {
            y0 = bt | 1w1;
        }
        bt = y0;
        tmp = 1w1;
        bt = 1w0;
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
