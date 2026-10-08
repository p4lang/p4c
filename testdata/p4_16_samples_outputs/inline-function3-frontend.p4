control p(inout bit<1> bt, in bit<1> bt2, in bit<1> bt3) {
    @name("p.y3") bit<1> y3_0;
    @name("p.tmp") bit<1> tmp;
    @name("p.tmp_0") bit<1> tmp_0;
    @name("p.tmp_1") bit<1> tmp_1;
    @name("p.y0") bit<1> y0;
    @name("p.y1") bit<1> y1;
    @name("p.y2") bit<1> y2;
    @name("p.a_0") bit<1> a;
    @name("p.retval") bit<1> retval;
    @name("p.inlinedRetval") bit<1> inlinedRetval_1;
    @name("p.a_1") bit<1> a_2;
    @name("p.retval") bit<1> retval_1;
    @name("p.inlinedRetval_0") bit<1> inlinedRetval_2;
    @name("p.b") action b() {
        a = bt2;
        retval = a + 1w1;
        inlinedRetval_1 = retval;
        tmp_0 = inlinedRetval_1;
        a_2 = bt3;
        retval_1 = a_2 + 1w1;
        inlinedRetval_2 = retval_1;
        tmp_1 = inlinedRetval_2;
        y0 = bt;
        y1 = tmp_0;
        y2 = tmp_1;
        if (y1 > 1w0) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        y3_0 = tmp;
        if (y3_0 == 1w1) {
            y0 = 1w0;
        } else if (y0 != 1w1) {
            y0 = y2 | y1;
        }
        bt = y0;
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

control simple<T>(inout T arg, in T brg, in T crg);
package m<T>(simple<T> pipe);
m<bit<1>>(p()) main;
