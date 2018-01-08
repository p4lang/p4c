control ctrl() {
    @name("e") action e_0() {
        exit;
    }
    @name("f") action f_0() {
    }
    @name("t") table t {
        actions = {
            e_0();
            f_0();
        }
        default_action = e_0();
    }
    apply {
        switch (t.apply().action_run) {
            e_0: {
                t.apply();
            }
            f_0: {
                t.apply();
            }
        }

    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

