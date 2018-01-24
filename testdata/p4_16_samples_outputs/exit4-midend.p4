control ctrl() {
    bool hasExited;
    bool tmp_0;
    @name("ctrl.e") action e_0() {
        hasExited = true;
    }
    @name("ctrl.t") table t {
        actions = {
            e_0();
        }
        default_action = e_0();
    }
    @hidden action act() {
        tmp_0 = true;
    }
    @hidden action act_0() {
        tmp_0 = false;
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
        if (t.apply().hit) 
            tbl_act_0.apply();
        else 
            tbl_act_1.apply();
        if (!hasExited) 
            if (tmp_0) 
                t.apply();
            else 
                t.apply();
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

