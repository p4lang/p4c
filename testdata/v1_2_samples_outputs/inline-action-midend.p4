control p(inout bit<1> bt) {
    action a(inout bit<1> y0) {
        bool hasReturned_0 = false;
        y0 = y0 | 1w1;
    }
    action b() {
        bool hasReturned_1 = false;
        a(bt);
        a(bt);
    }
    table t() {
        actions = {
            b;
        }
        default_action = b;
    }

    apply {
        bool hasReturned = false;
        t.apply();
    }
}

package m(p pipe);
m(p()) main;
