control p(inout bit<1> bt, in bit<1> bt2, in bit<1> bt3) {
    @name("p.y3") bit<1> y3_0;
    @name("p.tmp") bit<1> tmp;
    @name("p.y0") bit<1> y0;
    @name("p.a_0") bit<1> a;
    @name("p.retval") bit<1> retval;
    @name("p.inlinedRetval") bit<1> inlinedRetval_2;
    @name("p.a_1") bit<1> a_3;
    @name("p.retval") bit<1> retval_1;
    @name("p.inlinedRetval_0") bit<1> inlinedRetval_3;
    @name("p.a_2") bit<1> a_4;
    @name("p.retval") bit<1> retval_2;
    @name("p.inlinedRetval_1") bit<1> inlinedRetval_4;
    @name("p.b") action b() {
        y0 = bt;
        a = bt2;
        retval = a + 1w1;
        inlinedRetval_2 = retval;
        if (inlinedRetval_2 > 1w0) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        y3_0 = tmp;
        if (y3_0 == 1w1) {
            y0 = 1w0;
        } else if (y0 != 1w1) @inlinedFrom("foo") {
            a_3 = bt2;
            retval_1 = a_3 + 1w1;
            inlinedRetval_3 = retval_1;
            a_4 = bt3;
            retval_2 = a_4 + 1w1;
            inlinedRetval_4 = retval_2;
            y0 = inlinedRetval_4 | inlinedRetval_3;
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
