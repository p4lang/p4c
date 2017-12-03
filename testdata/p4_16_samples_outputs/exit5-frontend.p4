control ctrl() {
    @name("e") action e_0() {
        exit;
    }
    @name("f") action f_0() {
    }
    @name("t") table t_0 {
        actions = {
            e_0();
            f_0();
        }
        default_action = e_0();
    }
    apply {
        switch (t_0.apply().action_run) {
            e_0: {
                t_0.apply();
            }
            f_0: {
                t_0.apply();
            }
        }

    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

