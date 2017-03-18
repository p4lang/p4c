control ctrl() {
    bool hasExited;
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
    }
    table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        if (t.apply().hit)
            if (!hasExited)
                t.apply();
        else
            if (!hasExited)
                t.apply();
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
