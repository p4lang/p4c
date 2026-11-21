bit<1> foo(in bit<1> a) {
    return a + 1;
}
control p(inout bit<1> bt) {
    action a(inout bit<1> y0, bit<1> y1) {
        bit<1> y2 = (y1 > 0 ? 1w1 : 0);
        if (y2 == 1) {
            y0 = 0;
        } else if (y1 != 1) {
            y0 = y0 | 1w1;
        }
    }
    action b() {
        a(bt, foo(bt));
        a(bt, 1);
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

control simple<T>(inout T arg);
package m<T>(simple<T> pipe);
m(p()) main;
