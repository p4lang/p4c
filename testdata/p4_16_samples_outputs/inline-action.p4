control p(inout bit<1> bt) {
    action a(inout bit<1> y0) {
        y0 = y0 | 1w1;
    }
    action b() {
        a(bt);
        a(bt);
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

