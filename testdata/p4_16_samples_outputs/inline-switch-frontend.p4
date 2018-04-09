control d(out bit<32> x) {
    @name("d.cinst.a1") action cinst_a1() {
    }
    @name("d.cinst.a2") action cinst_a2() {
    }
    @name("d.cinst.t") table cinst_t_0 {
        actions = {
            cinst_a1();
            cinst_a2();
        }
        default_action = cinst_a1();
    }
    apply {
        {
            bool cinst_hasReturned_0 = false;
            switch (cinst_t_0.apply().action_run) {
                cinst_a1: 
                cinst_a2: {
                    cinst_hasReturned_0 = true;
                }
                default: {
                    cinst_hasReturned_0 = true;
                }
            }

        }
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

