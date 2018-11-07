control ctrl() {
    bool hasExited;
    @name("ctrl.e") action e() {
        hasExited = true;
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
    @hidden action act() {
        hasExited = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        switch (t_0.apply().action_run) {
            e: {
                if (!hasExited) 
                    t_0.apply();
            }
            f: {
                if (!hasExited) 
                    t_0.apply();
            }
        }

    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

