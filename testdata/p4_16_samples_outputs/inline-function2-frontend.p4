control p(inout bit<1> bt) {
    @name("p.y2") bit<1> y2_0;
    @name("p.tmp") bit<1> tmp;
    @name("p.tmp_0") bit<1> tmp_0;
    @name("p.y0") bit<1> y0;
    @name("p.y1") bit<1> y1;
    @name("p.y0") bit<1> y0_1;
    @name("p.y1") bit<1> y1_1;
    @name("p.a_0") bit<1> a;
    @name("p.retval") bit<1> retval;
    @name("p.inlinedRetval") bit<1> inlinedRetval_0;
    @name("p.b") action b() {
        a = bt;
        retval = a + 1w1;
        inlinedRetval_0 = retval;
        tmp_0 = inlinedRetval_0;
        y0 = bt;
        y1 = tmp_0;
        if (y1 > 1w0) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        y2_0 = tmp;
        if (y2_0 == 1w1) {
            y0 = 1w0;
        } else if (y1 != 1w1) {
            y0 = y0 | 1w1;
        }
        bt = y0;
        y0_1 = bt;
        y1_1 = 1w1;
        if (y1_1 > 1w0) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        y2_0 = tmp;
        if (y2_0 == 1w1) {
            y0_1 = 1w0;
        } else if (y1_1 != 1w1) {
            y0_1 = y0_1 | 1w1;
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
