control d(out bit<32> x) {
    @name("x_0") bit<32> x_0_0;
    @name("hasReturned") bool hasReturned;
    @name("cinst.a1") action cinst_a1() {
    }
    @name("cinst.a2") action cinst_a2() {
    }
    @name("cinst.t") table cinst_t() {
        actions = {
            cinst_a1;
            cinst_a2;
        }
        default_action = cinst_a1;
    }

    apply {
        bool hasExited = false;
        {
            hasReturned = false;
            switch (cinst_t.apply().action_run) {
                cinst_a1: 
                cinst_a2: {
                    return;
                }
                default: {
                    return;
                }
            }

            ;
            x = x_0_0;
        }
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
