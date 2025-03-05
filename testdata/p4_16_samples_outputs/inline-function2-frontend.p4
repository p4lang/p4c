control p(inout bit<1> bt) {
    @name("p.y2") bit<1> y2_0;
    @name("p.tmp") bit<1> tmp;
    @name("p.y0") bit<1> y0;
    @name("p.y0") bit<1> y0_1;
    @name("p.a_0") bit<1> a;
    @name("p.retval") bit<1> retval;
    @name("p.a_1") bit<1> a_2;
    @name("p.retval") bit<1> retval_1;
    @name("p.b") action b() {
        y0 = bt;
        a = bt;
        retval = a + 1w1;
        if (retval > 1w0) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        y2_0 = tmp;
        if (y2_0 == 1w1) {
            y0 = 1w0;
        } else @inlinedFrom("foo") {
            a_2 = bt;
            retval_1 = a_2 + 1w1;
            if (retval_1 != 1w1) {
                y0 = y0 | 1w1;
            }
        }
        bt = y0;
        y0_1 = bt;
        tmp = 1w1;
        y2_0 = tmp;
        if (y2_0 == 1w1) {
            y0_1 = 1w0;
        } else {
            ;
        }
        bt = y0_1;
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
