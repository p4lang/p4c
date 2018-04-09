control ctrl() {
    bool hasExited;
    @name("ctrl.e") action e_0() {
        hasExited = true;
    }
    @name("ctrl.f") action f_0() {
    }
    @name("ctrl.t") table t {
        actions = {
            e_0();
            f_0();
        }
        default_action = e_0();
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
        switch (t.apply().action_run) {
            e_0: {
                if (!hasExited) 
                    t.apply();
            }
            f_0: {
                if (!hasExited) 
                    t.apply();
            }
        }

    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

