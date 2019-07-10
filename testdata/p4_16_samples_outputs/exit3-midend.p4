control ctrl(out bit<32> c) {
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
        c = 32w2;
    }
    @hidden action act_0() {
        c = 32w5;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        t_0.apply();
        if (!hasExited) {
            tbl_act_0.apply();
        }
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;

