control ctrl() {
    @name("ctrl.e") action e() {
        exit;
    }
    @name("ctrl.f") action f() {
    }
    @name("ctrl.t") table t_0 {
        actions = {
            e();
            f();
        }
        default_action = e();
    }
    apply {
        switch (t_0.apply().action_run) {
            e: {
                t_0.apply();
            }
            f: {
                t_0.apply();
            }
        }

    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

