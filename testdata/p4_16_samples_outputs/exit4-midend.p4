control ctrl() {
    bool hasExited;
    @name("ctrl.e") action e() {
        hasExited = true;
    }
    @name("ctrl.t") table t_0 {
        actions = {
            e();
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
        if (t_0.apply().hit) {
            if (hasExited) {
                ;
            } else {
                t_0.apply();
            }
        } else if (hasExited) {
            ;
        } else {
            t_0.apply();
        }
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
