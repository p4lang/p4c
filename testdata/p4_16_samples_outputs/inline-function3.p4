bit<1> foo(in bit<1> a) {
    return a + 1;
}
control p(inout bit<1> bt, in bit<1> bt2, in bit<1> bt3) {
    action a(inout bit<1> y0, bit<1> y1, bit<1> y2) {
        bit<1> y3 = (y1 > 0 ? 1w1 : 0);
        if (y3 == 1) {
            y0 = 0;
        } else if (y0 != 1) {
            y0 = y2 | y1;
        }
    }
    action b() {
        a(bt, foo(bt2), foo(bt3));
    }
    table t {
        actions = {
            b;
        }
        default_action = b;
    }
    apply {
        t.apply();
    }
}

control simple<T>(inout T arg, in T brg, in T crg);
package m<T>(simple<T> pipe);
m(p()) main;
