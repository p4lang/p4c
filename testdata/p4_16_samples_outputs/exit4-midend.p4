control ctrl() {
    bool hasExited;
    bool tmp_0;
    @name("e") action e_0() {
        hasExited = true;
    }
    @name("t") table t {
        actions = {
            e_0();
        }
        default_action = e_0();
    }
    action act() {
        hasExited = false;
        tmp_0 = t.apply().hit;
    }
    table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
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
