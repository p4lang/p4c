control ctrl() {
    bool hasExited;
    bool tmp;
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
        tmp = true;
    }
    @hidden action act_0() {
        tmp = false;
    }
    @hidden action act_1() {
        hasExited = false;
    }
    @hidden table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (t_0.apply().hit) {
            tbl_act_0.apply();
        } else {
            tbl_act_1.apply();
        }
        if (!hasExited) {
            if (tmp) {
                t_0.apply();
            } else {
                t_0.apply();
            }
        }
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

