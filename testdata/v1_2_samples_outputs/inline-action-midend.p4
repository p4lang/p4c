control p(inout bit<1> bt) {
    action b() {
        @name("y0_0") bit<1> y0_0_0;
        @name("y0_1") bit<1> y0_1_0;
        {
            y0_0_0 = bt;
            y0_0_0 = y0_0_0 | 1w1;
            bt = y0_0_0;
        }
        {
            y0_1_0 = bt;
            y0_1_0 = y0_1_0 | 1w1;
            bt = y0_1_0;
        }
    }
    table t() {
        actions = {
            b;
        }
        default_action = b;
    }

    apply {
        bool hasExited = false;
        t.apply();
    }
}

package m(p pipe);
m(p()) main;
